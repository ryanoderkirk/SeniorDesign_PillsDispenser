/*
 * Filesystem implementation logic
 */
#include "filesystem.h"
#include "lfs.h"
#include "logging.h"
#include "main.h"
#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"

#define FLASH_CMD_WREN 0x06 // Write Enable Instruction
#define FLASH_CMD_READ_SR1 0x05
#define FLASH_CMD_PAGE_READ 0x03
#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_DEVICE_ID 0x90A

#define PINCODE_SIZE 4

osMutexId_t filesystemMutex;

const osMutexAttr_t filesystemMutex_attributes = {
  "filesystemMutex",                     // Name for debugging
  osMutexRecursive | osMutexPrioInherit, // Allow recursive calls and priority inheritance
  NULL,                                  // Memory for control block
  0U                                     // Size of control block
};

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

static const char doseFilePath[] = "/config/doses";
static const char pinCodeFilePath[] = "/config/pincode";

// Send write enable command. Return 0 if flash chip is ready to be written to
static int flash_write_enable(void) {
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
    LogError("Timeout occured. WEL bit not set");
    return -1; // Timeout. The chip refused to enable writes.
}

// Blocks until the flash chip is completely idle
static int flash_wait_busy(void) {
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
    LogError("Timeout occured. WIP bit not set, flash is busy");
    return -1; // Timeout
}

// Read a region in a block. Negative error codes are propagated
// to the user.
int block_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    osMutexId_t* flashMutex = getFlashMutex();

    if (osMutexAcquire(*flashMutex, 1000) == osOK) {
        // Wait to make sure flash is available to be read
        if (flash_wait_busy() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        uint32_t addr = (block * c->block_size) + off;

        // 260 bytes covers the 4-byte command + max 256-byte LittleFS read
        static uint8_t tx_buf[260];
        static uint8_t rx_buf[260];

        // Wipe transmit buffer to avoid sending old data
        memset(tx_buf, 0, sizeof(tx_buf));

        // Build the read command header
        tx_buf[0] = FLASH_CMD_PAGE_READ;
        tx_buf[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        tx_buf[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        tx_buf[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        // Transmit message over SPI
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4 + size, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            LogError("Read command failed");
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        // Skip first 4 bytes (first 4 bytes was the address
        memcpy(buffer, &rx_buf[4], size);

        osMutexRelease(*flashMutex);
        return LFS_ERR_OK;
    } else {
        LogError("Mutex busy");
        return LFS_ERR_IO;
    }
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.

int block_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {

    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    osMutexId_t* flashMutex = getFlashMutex();
    if (osMutexAcquire(*flashMutex, 1000) == osOK) {

        // Ensure no write in progress
        if (flash_wait_busy() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        // Confirm the WEL bit is actually 1
        if (flash_write_enable() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
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

        // ONE SINGLE unbroken transmission to prevent the H5 SPE toggle glitch
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4 + size, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            LogError("Failed to transmit program buffer to flash memory");
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        if (flash_wait_busy() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        osMutexRelease(*flashMutex);
        return LFS_ERR_OK;

    } else {
        LogError("Mutex busy");
        return LFS_ERR_IO;
    }
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int block_erase(const struct lfs_config *c, lfs_block_t block) {
    SPI_HandleTypeDef *hspi = getFlashSPIHandle();

    osMutexId_t* flashMutex = getFlashMutex();

    if (osMutexAcquire(*flashMutex, 1000) == osOK) {
        if (flash_wait_busy() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        if (flash_write_enable() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        uint32_t addr = (block * c->block_size);

        static uint8_t tx_buf[4];
        static uint8_t rx_buf[4]; 

        tx_buf[0] = FLASH_CMD_SECTOR_ERASE;
        tx_buf[1] = (addr >> 16) & 0xFF; // Address High Byte (MSB)
        tx_buf[2] = (addr >> 8)  & 0xFF; // Address Middle Byte
        tx_buf[3] = (addr)       & 0xFF; // Address Low Byte (LSB)

        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 1000) != HAL_OK) {
            HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(*flashMutex);
            LogError("Erase command buffer failed to send");
            return LFS_ERR_IO;
        }
        HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET);

        if (flash_wait_busy() != 0) {
            osMutexRelease(*flashMutex);
            return LFS_ERR_IO;
        }

        osMutexRelease(*flashMutex);
        return LFS_ERR_OK;

    } else {
        return LFS_ERR_IO;
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
  //create mutex for filesystem functions
  filesystemMutex = osMutexNew(&filesystemMutex_attributes);

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

  const char *conf_files[] = {"/config/one", "/config/two", "/config/three",
                              "/config/four"};
  for (int i = 0; i < 4; i++) {
    // LFS_O_CREAT | LFS_O_RDWR ensures file exists without wiping it if it does
    result = lfs_file_open(&lfs, &file, conf_files[i], LFS_O_CREAT | LFS_O_RDWR);
    if (result < 0) {
      return -1;
    }
    result = lfs_file_close(&lfs, &file);
    if (result < 0) {
        return -1;
    }
  }

  result = lfs_file_open(&lfs, &file, doseFilePath, LFS_O_CREAT | LFS_O_RDWR);
  if (result < 0) {
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result < 0) {
      return -1;
  }

    return 0;
}

int filetest() {
    // only for testing
    return 0;
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
  if (filesystemMutex == NULL) {
    LogDebug("Filesystem mutex not created yet!");
    return -1;
  }
  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;

  uint8_t logName[7];
  get_rtc_YYMMDD(logName);

  char fullPath[64];
  snprintf(fullPath, sizeof(fullPath), "/logs/%s", logName);

  result = lfs_file_open(&lfs, &file, fullPath, LFS_O_CREAT | LFS_O_RDWR);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

int listLogFiles() {
  if (filesystemMutex == NULL) {
    LogDebug("Filesystem mutex not created yet!");
    return -1;
  }
  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }


  int result = 0;
  lfs_dir_t dir = {0};
  struct lfs_info info = {0};
  result = lfs_dir_open(&lfs, &dir, "/logs/");

  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  LogInfo("Log files...\n\n");

  while (lfs_dir_read(&lfs, &dir, &info) > 0) {
	  if (info.name[0] == '.') {
		  continue;
	  }
	  LogInfo("%s\n", info.name);
  }

  lfs_dir_close(&lfs, &dir);

  osMutexRelease(filesystemMutex);
  return 0;
}

int deleteAllLogs() {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = lfs_remove(&lfs, "/logs");

  // Recreate the folder so initDailyLog() doesn't fail later
  lfs_mkdir(&lfs, "/logs");

  osMutexRelease(filesystemMutex);
  return 0;
}

// write a log to the current day's log file
int writeLog(LogEntry_t *log) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;

  char filePath[20] = "/logs/YYMMDD";
  get_rtc_YYMMDD(filePath + 6);

  result = lfs_file_open(&lfs, &file, filePath,
                         LFS_O_CREAT | LFS_O_RDWR | LFS_O_APPEND);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_write(&lfs, &file, log, sizeof(LogEntry_t));
  if (result != sizeof(LogEntry_t)) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

// Read the most recent log
int readLog(LogEntry_t *log) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;

  char filePath[20] = "/logs/YYMMDD";
  get_rtc_YYMMDD(filePath + 6);

  result = lfs_file_open(&lfs, &file, filePath, LFS_O_RDONLY);
  if (result != 0) {
    LogDebug("file not yet created");
    osMutexRelease(filesystemMutex);
    return -1;
  }

  lfs_soff_t size = lfs_file_size(&lfs, &file);
  if (size < (lfs_soff_t)sizeof(LogEntry_t)) {
    lfs_file_close(&lfs, &file);
    // File too small/empty
    osMutexRelease(filesystemMutex);
    return -2;
  }

  result = lfs_file_seek(&lfs, &file, -sizeof(LogEntry_t), LFS_SEEK_END);
  if (result < 0) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_read(&lfs, &file, log, sizeof(LogEntry_t));
  if (result != sizeof(LogEntry_t)) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}


int writeConfig(Config_t* config) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

    int result = 0;
    const char *config_files[] = {"/config/one", "/config/two", "/config/three", "/config/four"};
    if (config->channel > 4 || config->channel < 1) {
        osMutexRelease(filesystemMutex);
        return -1;
    }

    result = lfs_file_open(&lfs, &file, config_files[config->channel - 1],
                           LFS_O_CREAT | LFS_O_RDWR | LFS_O_TRUNC);
    if (result != 0) {
      osMutexRelease(filesystemMutex);
      return -1;
    }

    result = lfs_file_write(&lfs, &file, config, sizeof(Config_t));
    if (result != sizeof(Config_t)) {
      lfs_file_close(&lfs, &file);
      osMutexRelease(filesystemMutex);
      return -1;
    }

    result = lfs_file_close(&lfs, &file);
    if (result != 0) {
      osMutexRelease(filesystemMutex);
      return -1;
    }

    osMutexRelease(filesystemMutex);
    return 0;
}

int readConfig(Config_t* config, int channel) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;
  const char *config_files[] = {"/config/one", "/config/two", "/config/three",
                             "/config/four"};
  if (channel > 4 || channel < 1) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_open(&lfs, &file, config_files[channel - 1], LFS_O_RDONLY);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
    }

  lfs_soff_t size = lfs_file_size(&lfs, &file);
  if (size < (lfs_soff_t)sizeof(Config_t)) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    // File too small/empty
    return -2;
  }

  result = lfs_file_read(&lfs, &file, config, sizeof(Config_t));

  int close_res = lfs_file_close(&lfs, &file);

  if (result != sizeof(Config_t) || close_res < 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

int clearConfig(int channel) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;
  const char *config_files[] = {"/config/one", "/config/two", "/config/three",
                             "/config/four"};
  if (channel > 4 || channel < 1) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_remove(&lfs, config_files[channel - 1]);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
    }

  osMutexRelease(filesystemMutex);
  return 0;
}

int writeDose(Dosage_t *config) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;

  result = lfs_file_open(&lfs, &file, doseFilePath,
                         LFS_O_CREAT | LFS_O_WRONLY | LFS_O_APPEND);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  lfs_soff_t size = lfs_file_size(&lfs, &file);
  if (size >= 5 * sizeof(Dosage_t)) {
    lfs_file_close(&lfs, &file);
    // File already has max amount of doses!
    osMutexRelease(filesystemMutex);
    return -2;
  }

  result = lfs_file_write(&lfs, &file, config, sizeof(Dosage_t));
  if (result != sizeof(Dosage_t)) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

int writePincode(const uint8_t* pincode) {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = 0;

  result = lfs_file_open(&lfs, &file, pinCodeFilePath,
                         LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_write(&lfs, &file, pincode, PINCODE_SIZE);
  if (result != PINCODE_SIZE) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}


int readPincode(uint8_t* pincode) {

  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  int result = lfs_file_open(&lfs, &file, pinCodeFilePath , LFS_O_RDONLY);
  if (result != 0) {
    // file not yet created
    osMutexRelease(filesystemMutex);
    return -1;
  }

  lfs_soff_t size = lfs_file_size(&lfs, &file);
  if (size < PINCODE_SIZE) {
    lfs_file_close(&lfs, &file);
    // File too small/empty
    osMutexRelease(filesystemMutex);
    return -2;
  }

  result = lfs_file_read(&lfs, &file, pincode, PINCODE_SIZE);
  if (result != PINCODE_SIZE) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

int readDoses(Dosage_t *doses, uint32_t bufferSize) {

  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }

  // System only supports 5 dosage times
  if (bufferSize > 5 || bufferSize < 1) {
    osMutexRelease(filesystemMutex);
    return -3;
  }
  int result = 0;

  result = lfs_file_open(&lfs, &file, pinCodeFilePath , LFS_O_RDONLY);
  if (result != 0) {
    // file not yet created
    osMutexRelease(filesystemMutex);
    return -1;
  }

  lfs_soff_t size = lfs_file_size(&lfs, &file);
  if (size < (lfs_soff_t) sizeof(Dosage_t)) {
    lfs_file_close(&lfs, &file);
    // File too small/empty
    osMutexRelease(filesystemMutex);
    return -2;
  }

    // return max amount of data possible: if buffer size if bigger that number doses, return number doses. If numberDoses is bigger, return bufferSize
  int numberDoses = size / sizeof(Dosage_t);
  int dosesRead = -1;

  // read buffersize amount of doses
  if (numberDoses > bufferSize) {
    dosesRead = bufferSize;
  }
  // read numberDoses amount of doses
  else {
    dosesRead = numberDoses;
  }

  result = lfs_file_seek(&lfs, &file, -sizeof(Dosage_t) * dosesRead,
                         LFS_SEEK_END);
  if (result < 0) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_read(&lfs, &file, doses, dosesRead * sizeof(Dosage_t));
  if (result != dosesRead * sizeof(Dosage_t)) {
    lfs_file_close(&lfs, &file);
    osMutexRelease(filesystemMutex);
    return -1;
  }

  result = lfs_file_close(&lfs, &file);
  if (result != 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return dosesRead;
}

int clearDoses() {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }
  int result = lfs_remove(&lfs, doseFilePath);

  // If the file is already gone (not found), return success
  if (result == LFS_ERR_NOENT) {
    osMutexRelease(filesystemMutex);
    return 0;
  }

  // If result is negative (other than NOT_FOUND), it's a real filesystem error
  if (result < 0) {
    osMutexRelease(filesystemMutex);
    return -1;
  }

  osMutexRelease(filesystemMutex);
  return 0;
}

int countDoses() {
  if (filesystemMutex == NULL)
    return -1;

  if (osMutexAcquire(filesystemMutex, 500U) != osOK) {
    LogDebug("Could not acquire filesystem mutex!");
    return -2;
  }
    int result = lfs_file_open(&lfs, &file, doseFilePath, LFS_O_RDONLY);
    if (result < 0) {
      osMutexRelease(filesystemMutex);
      return 0; // If file doesn't exist, count is 0
    }

    lfs_soff_t size = lfs_file_size(&lfs, &file);
    lfs_file_close(&lfs, &file);

    osMutexRelease(filesystemMutex);
    return (int)(size / sizeof(Dosage_t));
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
