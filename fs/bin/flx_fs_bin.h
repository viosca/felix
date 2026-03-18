#ifndef FELIX_FS_BINFS_H
#define FELIX_FS_BINFS_H

/**
 * @brief Initializes the /bin filesystem and mounts it to the VFS root.
 * @return 0 on success, non-zero on failure.
 */
int flx_fs_bin_init(void);

/**
 * @brief Registers a C function as an executable file in the /bin directory.
 * @param name The name of the command (e.g., "ls")
 * @param main_func The standard POSIX entry point for the command
 * @return 0 on success, -1 if the /bin directory is full.
 */
int flx_fs_bin_register(const char *name, int (*main_func)(int argc, char **argv));


#endif /* FELIX_FS_BINFS_H */
