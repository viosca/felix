#include <sys/sysinfo.h>

int sysinfo(struct sysinfo *info) {
    if (!info) return -1;

    /* Return dummy data so tools don't crash or hang */
    info->uptime = 0;
    info->totalram = 1024 * 1024;
    info->freeram = 1024 * 512;
    info->procs = 1;
    info->mem_unit = 1;

    return 0;
}
