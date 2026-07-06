/**
 * @file felix/fs/fatfs/ffsystem.c
 */
/*------------------------------------------------------------------------*/
/* Sample Code of OS Dependent Functions for FatFs                        */
/* (C)ChaN, 2024                                                          */
/* Customized for Felix OS                                                */
/*------------------------------------------------------------------------*/

#include "ff.h"
#include <stdlib.h> /* For standard malloc/free */
#include <time.h>   /* For POSIX time() and localtime() */
#include "kernel/kmalloc.h"

/* TODO: Include Felix OS specific mutex header here */
/* #include "felix_mutex.h" */

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/

void* ff_memalloc (
	UINT msize		/* Number of bytes to allocate */
)
{
    if (msize > 4096) {
        return NULL;
    }
	/* Felix OS uses standard C library malloc for heap allocations */
	return kmalloc(msize);
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
	void* mblock	/* Pointer to the memory block to free */
)
{
	/* Felix OS uses standard C library free */
	kfree(mblock);
}


#if FF_FS_REENTRANT	/* Mutex Control (Enabled for Felix OS) */
/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
/  synchronization object for the volume, such as semaphore and mutex.
/  When a 0 is returned, the f_mount() function fails with FR_INT_ERR.
*/

int ff_mutex_create (	/* Returns 1:Function succeeded, 0:Could not create the mutex */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	/* TODO: Hook this into Felix OS Mutex Creation */
	/* Example:
	 * OS_MUTEX* mutex = create_mutex();
	 * if (!mutex) return 0;
	 * Mutex[vol] = mutex;
	 */

	return 1; /* Return 1 (success) as a stub for now */
}


/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_mutex_create() function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

void ff_mutex_delete (	/* Returns 1:Function succeeded, 0:Could not delete due to any error */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	/* TODO: Hook this into Felix OS Mutex Deletion */
	/* Example:
	 * delete_mutex(Mutex[vol]);
	 */
}


/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_mutex_take (		/* Returns 1:Succeeded, 0:Timeout */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	/* TODO: Hook this into Felix OS Mutex Lock */
	/* Example:
	 * if (lock_mutex_with_timeout(Mutex[vol], FF_FS_TIMEOUT) == TIMEOUT) {
	 * return 0;
	 * }
	 */

	return 1; /* Return 1 (success) as a stub for now */
}


/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_mutex_give (
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	/* TODO: Hook this into Felix OS Mutex Unlock */
	/* Example:
	 * unlock_mutex(Mutex[vol]);
	 */
}

#endif	/* FF_FS_REENTRANT */

/*------------------------------------------------------------------------*/
/* Get Current Time                                                       */
/*------------------------------------------------------------------------*/
/* This function is required because FF_FS_NORTC is 0.                    */
/* It returns the current time packed into a 32-bit DWORD.                */

DWORD get_fattime (void)
{
    time_t rawtime = time(NULL);

    /* time() now correctly reads from the kernel's flx_time_get_timeofday().
    /  However, if g_wall_clock_offset_ms is still 0 (OpenOCD hasn't set the time yet),
    /  rawtime will just be the uptime since 1970. We check if the time is before
    /  Jan 1, 2025 (1735689600). If it is, we use a safe fallback. */
    if (rawtime <= 0 || rawtime < 1735689600) {
        /* Hardcoded fallback: Jan 1, 2025, 00:00:00 */
        return    ((DWORD)(2025 - 1980) << 25)  /* Year 2025 */
                | ((DWORD)1 << 21)              /* Month 1 */
                | ((DWORD)1 << 16)              /* Mday 1 */
                | ((DWORD)0 << 11)              /* Hour 0 */
                | ((DWORD)0 << 5)               /* Min 0 */
                | ((DWORD)0 >> 1);              /* Sec 0 */
    }

    /* The time HAS been set! Convert POSIX time to MS-DOS FAT format. */
    struct tm * pt = localtime(&rawtime);

    return    ((DWORD)(pt->tm_year - 80) << 25)  /* tm_year is years since 1900. FatFs wants years since 1980 */
            | ((DWORD)(pt->tm_mon + 1) << 21)    /* tm_mon is 0-11, FatFs wants 1-12 */
            | ((DWORD)pt->tm_mday << 16)
            | ((DWORD)pt->tm_hour << 11)
            | ((DWORD)pt->tm_min << 5)
            | ((DWORD)(pt->tm_sec / 2));         /* FatFs stores seconds divided by 2 */
}
