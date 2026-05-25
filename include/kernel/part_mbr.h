/**
 * @file felix/include/kernel/part_mbr.h
 * @brief Master Boot Record (MBR) parsing subsystem.
 */
#ifndef PART_MBR_H
#define PART_MBR_H

#include <stdint.h>
#include "kernel/vfs.h"
#include "kernel/part_types.h" /* Drop the internal struct and use the decoupled 64-bit one */

/**
 * @brief Reads Sector 0, parses the MBR, and registers discovered partitions with the VFS.
 * @param raw_name    The name of the raw drive (e.g., "sd0").
 * @param major       The major device number assigned to this driver.
 * @param ops         The VFS block operations struct to bind to the partitions.
 * @param part_array  An array of FlxPartition_t structs to populate with the results.
 * @param read_sector Hardware-agnostic callback to read physical sectors.
 * @return 0 on success, or a negative error code on failure.
 */
int part_parse_mbr(const char *raw_name, int major, const FlxVfsOps_t *ops, FlxPartition_t *part_array, int (*read_sector)(uint64_t lba, uint8_t *buf));

#endif /* PART_MBR_H */
