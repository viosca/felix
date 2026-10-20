/**
 * @file felix/include/kernel/part_types.h
 * @brief Shared partition structures and types for storage parsers.
 */
#ifndef PART_TYPES_H
#define PART_TYPES_H

#include <stdint.h>

#define FLX_PART_TYPE_EMPTY       0x00
#define FLX_PART_TYPE_FAT12       0x01
#define FLX_PART_TYPE_FAT16       0x06
#define FLX_PART_TYPE_EXFAT       0x07
#define FLX_PART_TYPE_FAT32       0x0B
#define FLX_PART_TYPE_FAT32_LBA   0x0C
#define FLX_PART_TYPE_HSS_BOOT    0x48 /* HSS Bootloader Payload / BIOS Boot (Raw Binary) */
#define FLX_PART_TYPE_GPT         0xEE

/**
 * @brief Represents a single parsed partition's boundaries.
 * Upgraded to 64-bit to support drives > 2TB.
 */
typedef struct {
    uint8_t  type;         /* Partition Type (Mapped for legacy, otherwise 0xEE) */
    uint64_t lba_start;    /* Sector offset where the partition begins */
    uint64_t sector_count; /* Size of partition */
} FlxPartition_t;

#endif /* PART_TYPES_H */
