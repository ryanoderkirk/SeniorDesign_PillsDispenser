/*
 * Filesystem header
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "lfs.h"

typedef enum {
        dispenseTransaction,
        systemBoot
} LogType_t;

typedef struct __attribute__((packed)) {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    LogType_t logType;
    uint8_t one;
    uint8_t two;
    uint8_t three;
    uint8_t four;
    uint8_t five;
    uint8_t six;
} LogEntry_t;

typedef struct __attribute__((packed)) {
    uint8_t channel;
    uint8_t pillCount;
    uint8_t pillName[64];
} Config_t;


typedef struct __attribute__((packed)) {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t day; // Reserved for future day functionality. Could represent frequency of dispense, or each bit represents a day, etc.
    uint8_t pillOne[64];
    uint8_t pillOneCount;
    uint8_t pillTwo[64];
    uint8_t pillTwoCount;
    uint8_t pillThree[64];
    uint8_t pillThreeCount;
    uint8_t pillFour[64];
    uint8_t pillFourCount;
} Dosage_t;

// Mount file system if already formatted
// If not already formatted, format then mount
int filesystemInit();

// write an empty log file in the logs directory of name YYMMDD
int initDailyLog();

// Write the name of all log files to UART debug VCOM port
int listLogFiles();

// Delete all file in the logs directory
int deleteAllLogs();

// write a log to the current day's log file
int writeLog(LogEntry_t* log);

// Read the most recent log
int readLog(LogEntry_t* log);

int writeConfig(Config_t* config);

int readConfig(Config_t* config, int channel);

int writeDose(Dosage_t* config);

int readDoses(Dosage_t *doses, uint32_t numberLogs);

int clearDoses();

int countDoses();

// Fill timestamp of the log
int fillLogTimestamp(LogEntry_t* log);

// Read a region in a block. Negative error codes are propagated
// to the user.
int (block_read)(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int (block_program)(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int (block_erase)(const struct lfs_config *c, lfs_block_t block);

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int (block_sync)(const struct lfs_config *c);

#endif
