/**
 * @file felix/include/kernel/part_gpt.h
 * @brief GUID Partition Table (GPT) parsing subsystem.
 */
#ifndef PART_GPT_H
#define PART_GPT_H

#include <stdint.h>
#include "kernel/vfs.h"
/* Use the new decoupled types header instead of blk_manager.h */
#include "kernel/part_types.h"

/**
 * @brief Reads LBA 1, parses the GPT, and registers discovered partitions with the VFS.
 * @param raw_name       The name of the raw drive (e.g., "sd0").
 * @param major          The major device number assigned to this driver.
 * @param ops            The VFS block operations struct to bind to the partitions.
 * @param part_array     Array of FlxPartition_t structs to populate.
 * @param max_partitions The size of the part_array.
 * @param read_sector    Hardware-agnostic callback to read physical sectors.
 * @return 0 on success, or a negative error code if no valid GPT is found.
 */
int part_parse_gpt(const char *raw_name, int major, const FlxVfsOps_t *ops,
                   FlxPartition_t *part_array, int max_partitions,
                   int (*read_sector)(uint64_t lba, uint8_t *buf));

#endif /* PART_GPT_H */
