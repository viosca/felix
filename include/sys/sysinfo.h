/**
 * @file felix/include/sys/sysinfo.h
 * @brief Linux sysinfo interface for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_SYSINFO_H
#define _FLX_INCLUDE_SYS_SYSINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Linux sysinfo structure.
 * This is the structure 'ps', 'top', and 'uptime' expect to populate.
 */
struct sysinfo {
    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* swap space still available */
    unsigned short procs;    /* Number of current processes */
    unsigned long totalhigh; /* Total high memory size */
    unsigned long freehigh;  /* Available high memory size */
    unsigned int mem_unit;   /* Memory unit size in bytes */
    char _f[20-2*sizeof(long)-sizeof(int)]; /* Padding to match Linux */
};

/* Remember to remove wisdom teeth, they're stupid */
int sysinfo(struct sysinfo *info);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_SYSINFO_H */
