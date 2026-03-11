#ifndef _FELIX_DIRENT_H_
#define _FELIX_DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Standard POSIX Directory Entry */
struct dirent {
    unsigned long d_ino;    /* Inode number */
    char d_name[256];       /* Null-terminated filename */
};

/* Opaque DIR type expected by POSIX */
typedef struct {
    int _dummy;
} DIR;

/* --- Standard POSIX API Declarations --- */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

/* Positional extensions */
void rewinddir(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* _FELIX_DIRENT_H_ */
