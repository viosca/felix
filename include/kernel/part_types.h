/**
 * @file felix/include/kernel/part_types.h
 * @brief Shared partition structures and types for storage parsers.
 */
#ifndef PART_TYPES_H
#define PART_TYPES_H

#include <stdint.h>

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
