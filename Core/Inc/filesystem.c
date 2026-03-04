/*
 * Filesystem implementation logic
 */
#include "filesystem.h"
#include "logging.h"
#include "main.h"
#include <stdint.h>
#include <string.h>

#define FLASH_CMD_WREN 0x06 // Write Enable Instruction
#define FLASH_CMD_READ_SR1 0x05
#define FLASH_CMD_PAGE_READ 0x03
#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_DEVICE_ID 0x90A

// variables used by the filesystem
static lfs_t lfs;
static lfs_file_t file;

// configuration of the filesystem is provided by this struct
static const struct lfs_config cfg = {
    // block device operations
    .read = block_read,
    .prog = block_program,
    .erase = block_erase,
    .sync = block_sync,

    // block device configuration
    .read_size = 256,
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 512,
    .cache_size = 256,
    .lookahead_size = 16,
    .block_cycles = 500,
};

// Forces the flash chip to accept a write command
int flash_write_enable(void) {
    SPI_HandleTypeDef *hspi = getFlashSPIHandle();
    uint8_t wrenCmd = FLASH_CMD_WREN;
    uint8_t dummy_rx;
    
    uint8_t tx_buf[2] = {FLASH_CMD_READ_SR1, 0x00};
    uint8_t rx_buf[2] = {0};

    // 1. Send the Write Enable command
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(hspi, &wrenCmd, &dummy_rx, 1, 1000);
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

    // 2. Wait until the WEL bit (Bit 1, 0x02) is actually set!
    for (uint32_t i = 0; i < 100; i++) {
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 1000);
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        if ((rx_buf[1] & 0x02) != 0) {
            return 0; // Success! The chip is locked and loaded.
        }
        osDelay(100);
    }
    return -1; // Timeout. The chip refused to enable writes.
}

// Blocks until the flash chip is completely idle
int flash_wait_busy(void) {
    SPI_HandleTypeDef *hspi = getFlashSPIHandle();
    uint8_t tx_buf[2] = {FLASH_CMD_READ_SR1, 0x00};
    uint8_t rx_buf[2] = {0};

    // 500 loops * 5ms = 2.5 second absolute max timeout
    for (uint32_t i = 0; i < 500; i++) { 
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 1000);
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // Check if WIP bit (Bit 0) is clear
        if ((rx_buf[1] & 0x01) == 0) {
            return 0; // Chip is idle!
        }
        osDelay(5);
    }
    return -1; // Timeout
}

int32_t flashIsReady() {
  SPI_HandleTypeDef *hspi = getFlashSPIHandle();
  uint8_t tx_buf[2] = {FLASH_CMD_READ_SR1, 0x00};
  uint8_t rx_buf[2] = {0};

  // Write Enable
  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 2, 100) != HAL_OK) {
    HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
    return -1;
  }
  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

  return !(rx_buf[1] & 0x01);
}

// Read a region in a block. Negative error codes are propagated
// to the user.

int block_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    if (1) {
        // 1. Your instinct applied: Defensive wait before asserting the bus!
        if (flash_wait_busy() != 0) {
            return -1;
        }

        uint32_t addr = (block * c->block_size) + off;

        // 2. STATIC arrays to prevent the FreeRTOS stack overflow trap
        // 260 bytes covers the 4-byte command + max 256-byte LittleFS read
        static uint8_t tx_buf[260];
        static uint8_t rx_buf[260];

        // Wipe the transmit buffer so we don't accidentally send old garbage 
        memset(tx_buf, 0, sizeof(tx_buf));

        // 3. Build the read command header
        tx_buf[0] = FLASH_CMD_PAGE_READ;
        tx_buf[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        tx_buf[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        tx_buf[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // 4. ONE SINGLE unbroken transmission to prevent the H5 clock glitch!
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4 + size, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            return -2;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // 5. Extract the payload. 
        // We skip the first 4 bytes of rx_buf because they are just dummy 
        // responses sent by the flash chip while we were transmitting the command.
        memcpy(buffer, &rx_buf[4], size);

        return LFS_ERR_OK;
    } else {
        return -1;
    }
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.

int block_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    if (1) { // Placeholder for your osMutexAcquire
        
        // 1. Defensive wait: Ensure no background operations are running
        if (flash_wait_busy() != 0) {
            return -1;
        }

        // 2. Ironclad Write Enable: Confirm the WEL bit is actually 1
        if (flash_write_enable() != 0) {
            return -2;
        }

        uint32_t addr = (block * c->block_size) + off;
        
        // 3. STATIC unified buffers: Zero stack overhead!
        // 260 bytes covers the 4-byte command + max 256-byte LittleFS payload
        static uint8_t tx_buf[260];
        static uint8_t rx_buf[260]; 

        // 4. Build the unified payload
        tx_buf[0] = FLASH_CMD_PAGE_PROGRAM;
        tx_buf[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        tx_buf[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        tx_buf[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // Stitch the data payload right behind the address
        memcpy(&tx_buf[4], buffer, size);

        // 5. ONE SINGLE unbroken transmission to prevent the H5 SPE toggle glitch
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4 + size, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            return -3;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // 6. Block until the flash chip physically commits the bits
        if (flash_wait_busy() != 0) {
            return -4;
        }
        
        return LFS_ERR_OK;

    } else {
        return -1;
    }
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int block_erase(const struct lfs_config *c, lfs_block_t block) {
    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    if (1) { // Placeholder for your osMutexAcquire
        
        // 1. Defensive wait
        if (flash_wait_busy() != 0) {
            return -1;
        }

        // 2. Ironclad Write Enable
        if (flash_write_enable() != 0) {
            return -2;
        }

        uint32_t addr = (block * c->block_size);
        
        // 3. STATIC unified buffers (Keeps the memory footprint consistent and safe)
        static uint8_t tx_buf[4];
        static uint8_t rx_buf[4]; 

        tx_buf[0] = FLASH_CMD_SECTOR_ERASE;
        tx_buf[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        tx_buf[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        tx_buf[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // 4. Execute the erase command as a single Full-Duplex transaction
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            return -3;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // 5. Block until the sector is completely wiped back to 0xFF
        if (flash_wait_busy() != 0) {
            return -4;
        }
        
        return LFS_ERR_OK;

    } else {
        return -1;
    }
}

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int(block_sync)(const struct lfs_config *c) {

  // Currently always true because SPI transmission is blocking. Will need to
  // implement in the future when setting up DMA transmission
  return LFS_ERR_OK;
}

// Mount file system if already formatted
// If not already formatted, format then mount
int filesystemInit() {
  SPI_HandleTypeDef *hspi = getFlashSPIHandle();

  // Debug hardware: Read JEDEC ID (0x9F)
  uint8_t id_buf[4] = {0x9F, 0, 0, 0};
  uint8_t id_rx[4] = {0};
  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(hspi, id_buf, id_rx, 4, 1000);
  HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

  LogInfo("FLASH DEBUG: JEDEC = %02X %02X %02X\r\n", id_rx[1], id_rx[2],
          id_rx[3]);

  if (id_rx[1] == 0x00 && id_rx[2] == 0x00) {
    LogInfo("ERROR: Flash returning 0x00! Check MISO (PG9) wiring, VDDIO2, and "
            "Chip Select.\r\n");
  }

  // mount the filesystem
  int test = lfs_format(&lfs, &cfg);
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

int filetest() {
  for (int i = 0; i < 1000; i++) {

    // 1. Erase
    int err = block_erase(&cfg, 0);
    if (err !=0)
        return -1;

    // 3. Read
    uint8_t test_buf2[10] = {0}; // Init to 0 to flush stack garbage
    err = block_read(&cfg, 0, 0, test_buf2, 10);
    if (err !=0)
    	return -1;

    // 2. Program
    uint8_t write_buf[] = {4, 5, 6};
    err = block_program(&cfg, 0, 0, write_buf, 3);
    if (err !=0)
        return -1;

    // 3. Read
    uint8_t test_buf[10] = {0}; // Init to 0 to flush stack garbage
    err = block_read(&cfg, 0, 0, test_buf, 10);
    if (err !=0)
        return -1;
    if(test_buf[0] != 4)
    {
    	return -1;
    }
  }
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

  while (lfs_dir_read(&lfs, &dir, &info) > 0) {
    LogInfo("%s\n", info.name);
    // HAL_UART_Transmit(getDebugHandle(), (uint8_t*)info.name,
    // strlen(info.name), 100); HAL_UART_Transmit(getDebugHandle(),
    // (uint8_t*)"\r\n", 3, 100);
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

  while (lfs_dir_read(&lfs, &dir, &info) > 0) {
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
int writeLog(LogEntry_t *log) {
  int result = 0;

  char filePath[20] = "/logs/YYMMDD";
  get_rtc_YYMMDD(filePath + 6);

  result = lfs_file_open(&lfs, &file, filePath,
                         LFS_O_CREAT | LFS_O_RDWR | LFS_O_APPEND);
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
int readLog(LogEntry_t *log) {

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

int fillLogTimestamp(LogEntry_t *log) {
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
