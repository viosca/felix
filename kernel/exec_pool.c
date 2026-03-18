#include <stddef.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "kernel/binfmt.h"

/* Global limit for compiled-in executables across the entire OS (/bin, /sbin, etc.) */
#define FLX_MAX_SYS_EXECUTABLES 128

static FlxVfsNode_t    g_exec_nodes[FLX_MAX_SYS_EXECUTABLES];
static FlxLinkedExec_t g_exec_payloads[FLX_MAX_SYS_EXECUTABLES];
static int g_exec_count = 0;

FlxVfsNode_t* flx_vfs_create_exec_node(const char *name, int (*main_func)(int, char**)) {
    if (g_exec_count >= FLX_MAX_SYS_EXECUTABLES || !name || !main_func) {
        return NULL; /* Out of memory pool space */
    }

    int i = g_exec_count++;

    /* 1. Setup the Binary Format Payload */
    g_exec_payloads[i].magic[0] = FLX_MAGIC_0;
    g_exec_payloads[i].magic[1] = FLX_MAGIC_1;
    g_exec_payloads[i].magic[2] = FLX_MAGIC_2;
    g_exec_payloads[i].magic[3] = FLX_MAGIC_3;
    g_exec_payloads[i].type = FLX_BINFMT_TYPE_LINKED;
    g_exec_payloads[i].main_func = main_func;

    /* 2. Setup the VFS Node pointing to the payload */
    g_exec_nodes[i].name = name;
    g_exec_nodes[i].parent = NULL; /* Assigned by the folder that adopts it */
    g_exec_nodes[i].mode = S_IFREG | 0555; /* Read + Execute permissions */
    g_exec_nodes[i].size = sizeof(FlxLinkedExec_t);
    g_exec_nodes[i].data = &g_exec_payloads[i];

    return &g_exec_nodes[i];
}
