/*
 * Filesystem implementation logic
*/
#include "filesystem.h"
#include "logging.h"
#include "main.h"
#include <stdint.h>

#define FLASH_CMD_WREN  0x06 // Write Enable Instruction
#define FLASH_CMD_READ_SR1  0x05
#define FLASH_CMD_PAGE_READ  0x03
#define FLASH_CMD_PAGE_PROGRAM  0x02
#define FLASH_CMD_SECTOR_ERASE  0x20
#define FLASH_CMD_DEVICE_ID  0x90A

// variables used by the filesystem
static lfs_t lfs;
static lfs_file_t file;

// configuration of the filesystem is provided by this struct
static const struct lfs_config cfg = {
    // block device operations
    .read  = block_read,
    .prog  = block_program,
    .erase = block_erase,
    .sync  = block_sync,

    // block device configuration
    .read_size = 256,
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 512,
    .cache_size = 256,
    .lookahead_size = 16,
    .block_cycles = 500,
};


int32_t flashIsReady() {
    SPI_HandleTypeDef *hspi = getFlashSPIHandle();
    uint8_t statusRegister = 0;
    uint8_t cmd = FLASH_CMD_READ_SR1;

    // Write Enable
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(hspi, &cmd, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
        return -1;
    }
    if (HAL_SPI_Receive(hspi, &statusRegister, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
        return -1;
    }
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

    return !(statusRegister & 0x01);

}

// Read a region in a block. Negative error codes are propagated
// to the user.
int block_read (const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    osMutexId_t* flashMutex = getFlashMutex();
    if (osMutexAcquire(*flashMutex, 100) == osOK) {
        // 1. Calculate the physical address on the flash chip
        // Physical Address = (Block Number * Block Size) + Offset
        uint32_t addr = (block * c->block_size) + off;

        // 2. Prepare the Read Command (0x03) followed by the 24-bit address
        uint8_t cmd[4];
        cmd[0] = FLASH_CMD_PAGE_READ;               // Normal Read Instruction
        cmd[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        cmd[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        cmd[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // 3. Select the Flash Chip (/CS Low)
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);

        // 4. Send the 4-byte command/address packet
        if (HAL_SPI_Transmit(hspi, cmd, 4, 100) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        uint32_t err = 0;
        // 5. Receive the requested data
        // The SPI clock continues to toggle, and the flash shifts bits out on MISO
        if ((err = HAL_SPI_Receive(hspi, (uint8_t*)buffer, size, 1000)) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        // 6. Deselect the Flash Chip (/CS High)
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        osMutexRelease(*flashMutex);
        return LFS_ERR_OK;
    }
    else {
        return -1;
    }
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int (block_program)(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();
    uint8_t wrenCmd = FLASH_CMD_WREN;

    osMutexId_t* flashMutex = getFlashMutex();
    if (osMutexAcquire(*flashMutex, 100) == osOK) {
        // Write Enable
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(hspi, &wrenCmd, 1, 100) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // Physical Address = (Block Number * Block Size) + Offset
        uint32_t addr = (block * c->block_size) + off;
        uint8_t cmd[4];
        cmd[0] = FLASH_CMD_PAGE_PROGRAM;
        cmd[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        cmd[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        cmd[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // Write Command
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(hspi, cmd, 4, 10) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }
        if (HAL_SPI_Transmit(hspi, (uint8_t*)buffer, size, 100) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // Wait 200ms before failing write
        for(uint32_t i = 0;i<201;i++) {
            osDelay(10);
            int status = flashIsReady();
            if(status == 1) {
                osMutexRelease(*flashMutex);
                return LFS_ERR_OK;
            }
            if (status == -1) {
                break;
            }
        }

        osMutexRelease(*flashMutex);
        return LFS_ERR_IO;
    }
    else {
        return -1;
    }

}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int (block_erase)(const struct lfs_config *c, lfs_block_t block) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();
    uint8_t wrenCmd = FLASH_CMD_WREN;

    osMutexId_t* flashMutex = getFlashMutex();
    if (osMutexAcquire(*flashMutex, 100) == osOK) {
        // Write Enable
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(hspi, &wrenCmd, 1, 100) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        uint32_t addr = (block * c->block_size);
        uint8_t cmd[4];
        cmd[0] = FLASH_CMD_SECTOR_ERASE;
        cmd[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        cmd[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        cmd[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // Write Command
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(hspi, cmd, 4, 10) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // Wait 1000ms before failing write
        for(uint32_t i = 0;i<201;i++) {
            osDelay(5);
            int status = flashIsReady();
            if(status == 1) {
                osMutexRelease(*flashMutex);
                return LFS_ERR_OK;
            }
            if(status == -1) {
                break;
            }
        }
        osMutexRelease(*flashMutex);
        return LFS_ERR_IO;
    }
    else {
        return -1;
    }
}

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int (block_sync)(const struct lfs_config *c) {

    // Currently always true because SPI transmission is blocking. Will need to implement in the future when setting up DMA transmission
    return LFS_ERR_OK;
}

// Mount file system if already formatted
// If not already formatted, format then mount
int filesystemInit() {
    // mount the filesystem
    int result = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (result) {
        lfs_format(&lfs, &cfg);
        result = lfs_mount(&lfs, &cfg);
    }

    result = lfs_mkdir(&lfs, "config");
    if (!(result == LFS_ERR_EXIST || result == 0))
        return -1;

    result = lfs_mkdir(&lfs, "logs");
    if (!(result == LFS_ERR_EXIST || result == 0))
        return -1;

    return 0;
}

int initDailyLog() {

    int result = 0;
    lfs_dir_t dir = {0};

    uint8_t logName[7];
    get_rtc_YYMMDD(logName);

    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "/logs/%s", logName);

    result = lfs_file_open(&lfs, &file, fullPath, LFS_O_CREAT | LFS_O_RDWR);
    if (result != 0)
         return -1;

    result = lfs_file_close(&lfs, &file);
    if (result != 0)
        return -1;

    result = lfs_dir_close(&lfs, &dir);
    if (result != 0)
        return -1;

    return 0;

}

int listLogFiles() {

    int result = 0;
    lfs_dir_t dir = {0};
    struct lfs_info info = {0};
    result = lfs_dir_open(&lfs, &dir, "/logs/");

    if (result != 0)
        return -1;

    while(lfs_dir_read(&lfs, &dir, &info) > 0)
    {
        LogInfo("%s\n",  info.name);
        //HAL_UART_Transmit(getDebugHandle(), (uint8_t*)info.name, strlen(info.name), 100);
        //HAL_UART_Transmit(getDebugHandle(), (uint8_t*)"\r\n", 3, 100);
    }

    lfs_dir_close(&lfs, &dir);

    return 0;
}

int deleteAllLogs() {

    int result = 0;
    lfs_dir_t dir = {0};
    struct lfs_info info = {0};
    result = lfs_dir_open(&lfs, &dir, "/logs/");

    if (result != 0)
        return -1;

    while(lfs_dir_read(&lfs, &dir, &info) > 0)
    {
        if (info.name[0] == '.')
            continue;
        char fullPath[64];
        snprintf(fullPath, sizeof(fullPath), "/logs/%s", info.name);
        result = lfs_remove(&lfs, fullPath);
        if (result != 0)
            return -1;
    }

    lfs_dir_close(&lfs, &dir);

    return 0;

}


// write a log to the current day's log file
int writeLog(LogEntry_t* log) {
    int result = 0;

    char filePath[20] = "/logs/YYMMDD";
    get_rtc_YYMMDD(filePath + 6);

    result = lfs_file_open(&lfs, &file, filePath, LFS_O_CREAT | LFS_O_RDWR | LFS_O_APPEND);
    if (result != 0)
        return -1;

    result = lfs_file_write(&lfs, &file, log, sizeof(LogEntry_t));
    if (result != sizeof(LogEntry_t)) {
        lfs_file_close(&lfs, &file);
        return -1;
    }

    result = lfs_file_close(&lfs, &file);
    if (result != 0)
        return -1;

    return 0;
}

// Read the most recent log
int readLog(LogEntry_t* log) {

    int result = 0;

    char filePath[20] = "/logs/YYMMDD";
    get_rtc_YYMMDD(filePath + 6);

    result = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
    if (result != 0)
         // file not yet created
        return -1;

    lfs_soff_t size = lfs_file_size(&lfs, &file);
    if (size < (lfs_soff_t)sizeof(LogEntry_t)) {
        lfs_file_close(&lfs, &file);
        // File too small/empty
        return -2;
    }

    result = lfs_file_seek(&lfs, &file, -sizeof(LogEntry_t), LFS_SEEK_END);
    if (result < 0)
        return -1;

    result = lfs_file_read(&lfs, &file, log, sizeof(LogEntry_t));
    if (result != sizeof(LogEntry_t)) {
        lfs_file_close(&lfs, &file);
        return -1;
    }

    result = lfs_file_close(&lfs, &file);
    if (result != 0) {
        return -1;
    }

    return 0;
}


int fillLogTimestamp(LogEntry_t* log) {
    RTC_TimeTypeDef gTime;
    RTC_DateTypeDef gDate;

    // 1. Read Time FIRST (Locks shadow registers)
    HAL_RTC_GetTime(getRTCHandle(), &gTime, RTC_FORMAT_BIN);

    // 2. Read Date SECOND (Unlocks shadow registers)
    HAL_RTC_GetDate(getRTCHandle(), &gDate, RTC_FORMAT_BIN);

    log->year = gDate.Year;
    log->month = gDate.Month;
    log->day = gDate.Date;
    log->hour = gTime.Hours;
    log->min = gTime.Minutes;
    log->sec = gTime.Seconds;

    return 0;
}
