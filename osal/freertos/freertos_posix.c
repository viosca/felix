/**
 * @file felix/osal/freertos/freertos_posix.c
 * @brief Kconfig-controlled FreeRTOS+POSIX integration unit for Felix OS
 *
 * This compilation unit conditionally includes the selected FreeRTOS+POSIX
 * source files based on Kconfig (autoconf.h), keeping the upstream
 * FreeRTOS-Plus-POSIX submodule unmodified while excluding conflicting
 * clock/unistd implementations that Felix OS provides natively.
 */

#include "autoconf.h"

#if defined(CONFIG_FELIX_FREERTOS_PLUS_POSIX)

/* Core POSIX utilities and scheduler policies */
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_utils.c"
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_sched.c"

#if defined(CONFIG_FELIX_POSIX_PTHREAD)
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread.c"
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_mutex.c"
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_cond.c"
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_barrier.c"
#endif

#if defined(CONFIG_FELIX_POSIX_SEMAPHORE)
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_semaphore.c"
#endif

#if defined(CONFIG_FELIX_POSIX_MQUEUE)
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_mqueue.c"
#endif

#if defined(CONFIG_FELIX_POSIX_TIMER)
#include "../../external/FreeRTOS-Plus-POSIX/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_timer.c"
#endif

#endif /* CONFIG_FELIX_FREERTOS_PLUS_POSIX */
