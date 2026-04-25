/*
 * @file exec_pool.c
 */
#include "kernel/vfs.h"
#include <string.h>
#include <stdbool.h>

#define FLX_MAX_DYNAMIC_CMDS 32

/* Statically allocated memory pools for dynamic nodes */
static FlxVfsNode_t cmd_node_pool[FLX_MAX_DYNAMIC_CMDS];
static FlxLinkedExec_t cmd_payload_pool[FLX_MAX_DYNAMIC_CMDS];
static bool pool_in_use[FLX_MAX_DYNAMIC_CMDS] = {false};

FlxVfsNode_t* flx_vfs_create_exec_node(const char *name, int (*main_func)(int argc, char **argv), size_t explicit_size, const void *start_ptr, const void *end_ptr) {
    for (int i = 0; i < FLX_MAX_DYNAMIC_CMDS; i++) {
        if (!pool_in_use[i]) {
            pool_in_use[i] = true;

            /* 1. Setup Payload */
            cmd_payload_pool[i].magic[0] = FLX_MAGIC_0;
            cmd_payload_pool[i].magic[1] = FLX_MAGIC_1;
            cmd_payload_pool[i].magic[2] = FLX_MAGIC_2;
            cmd_payload_pool[i].magic[3] = FLX_MAGIC_3;
            cmd_payload_pool[i].type = FLX_BINFMT_TYPE_LINKED;
            cmd_payload_pool[i].size = explicit_size;
            cmd_payload_pool[i].start_ptr = start_ptr;
            cmd_payload_pool[i].end_ptr = end_ptr;
            cmd_payload_pool[i].main_func = main_func;

            /* 2. Setup Node */
            cmd_node_pool[i].name = name; /* Assume caller manages string memory */
            cmd_node_pool[i].kflags = 0;
            cmd_node_pool[i].parent = NULL;
            cmd_node_pool[i].mode = S_IFREG | 0555;
            cmd_node_pool[i].data = &cmd_payload_pool[i];
            cmd_node_pool[i].ops = NULL;

            return &cmd_node_pool[i];
        }
    }
    return NULL; /* Pool is full! */
}
