#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_POSIX)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <flx_test.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>

typedef struct {
    pthread_mutex_t mutex;
    volatile int counter;
    int iterations;
} PosixMutexTestCtx_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool ready;
} PosixCondTestCtx_t;

typedef struct {
    pthread_barrier_t barrier;
    int serial_thread_count;
    pthread_mutex_t mutex;
} PosixBarrierTestCtx_t;

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_return_test(void *arg) {
    uintptr_t val = (uintptr_t)arg;
    return (void *)(val + 0x1000UL);
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_mutex_test(void *arg) {
    PosixMutexTestCtx_t *ctx = (PosixMutexTestCtx_t *)arg;
    for (int i = 0; i < ctx->iterations; i++) {
        pthread_mutex_lock(&ctx->mutex);
        ctx->counter++;
        pthread_mutex_unlock(&ctx->mutex);
    }
    return NULL;
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_cond_test(void *arg) {
    PosixCondTestCtx_t *ctx = (PosixCondTestCtx_t *)arg;
    pthread_mutex_lock(&ctx->mutex);
    while (!ctx->ready) {
        pthread_cond_wait(&ctx->cond, &ctx->mutex);
    }
    pthread_mutex_unlock(&ctx->mutex);
    return (void *)0xCAFEULL;
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_barrier_test(void *arg) {
    PosixBarrierTestCtx_t *ctx = (PosixBarrierTestCtx_t *)arg;
    int rc = pthread_barrier_wait(&ctx->barrier);
    if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
        pthread_mutex_lock(&ctx->mutex);
        ctx->serial_thread_count++;
        pthread_mutex_unlock(&ctx->mutex);
    }
    return (void *)(intptr_t)rc;
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_sem_test(void *arg) {
    sem_t *sem = (sem_t *)arg;
    sem_post(sem);
    return NULL;
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void worker_timer_cb(union sigval sv) {
    sem_t *sem = (sem_t *)sv.sival_ptr;
    if (sem) {
        sem_post(sem);
    }
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
static void *worker_vfs_test(void *arg) {
    const char *path = (const char *)arg;
    const char *msg = "POSIX_PTHREAD_VFS_OK";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return (void *)-1L;
    }
    ssize_t written = write(fd, msg, strlen(msg));
    close(fd);
    return (written == (ssize_t)strlen(msg)) ? (void *)0L : (void *)-2L;
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
int flx_fs_test_cmd_posix(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS FreeRTOS+POSIX Verification Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. pthread_create & pthread_join (64-bit return values) */
    pthread_t t1, t2;
    void *ret1 = NULL, *ret2 = NULL;
    int rc = pthread_create(&t1, NULL, worker_return_test, (void *)0xCAFE0000UL)
           | pthread_create(&t2, NULL, worker_return_test, (void *)0xBEEF0000UL)
           | pthread_join(t1, &ret1)
           | pthread_join(t2, &ret2);
    FLX_TEST_CHECK(&t,
                   rc == 0 && (uintptr_t)ret1 == 0xCAFE1000UL && (uintptr_t)ret2 == 0xBEEF1000UL,
                   "pthread_create & pthread_join (64-bit retval: %p, %p)", ret1, ret2);

    /* 2. pthread_mutex synchronization */
    PosixMutexTestCtx_t mctx = { .counter = 0, .iterations = 200 };
    pthread_mutex_init(&mctx.mutex, NULL);
    if (pthread_create(&t1, NULL, worker_mutex_test, &mctx) == 0 &&
        pthread_create(&t2, NULL, worker_mutex_test, &mctx) == 0) {
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
    }
    pthread_mutex_destroy(&mctx.mutex);
    FLX_TEST_INT(&t, mctx.counter, "==", 400,
                 "pthread_mutex_lock/unlock contention (counter=%d/400)", mctx.counter);

    /* 3. pthread_cond signaling */
    PosixCondTestCtx_t cctx = { .ready = false };
    pthread_mutex_init(&cctx.mutex, NULL);
    pthread_cond_init(&cctx.cond, NULL);
    void *cond_res = NULL;
    if (pthread_create(&t1, NULL, worker_cond_test, &cctx) == 0) {
        usleep(5000); /* Ensure worker reached wait state */
        pthread_mutex_lock(&cctx.mutex);
        cctx.ready = true;
        pthread_cond_signal(&cctx.cond);
        pthread_mutex_unlock(&cctx.mutex);
        pthread_join(t1, &cond_res);
    }
    pthread_cond_destroy(&cctx.cond);
    pthread_mutex_destroy(&cctx.mutex);
    FLX_TEST_CHECK(&t, (uintptr_t)cond_res == 0xCAFEULL, "pthread_cond_wait / pthread_cond_signal");

    /* 4. pthread_barrier synchronization */
    PosixBarrierTestCtx_t bctx = { .serial_thread_count = 0 };
    pthread_barrier_init(&bctx.barrier, NULL, 3);
    pthread_mutex_init(&bctx.mutex, NULL);
    pthread_t tb[2];
    void *brc0 = (void *)-1L, *brc1 = (void *)-1L;
    if (pthread_create(&tb[0], NULL, worker_barrier_test, &bctx) == 0 &&
        pthread_create(&tb[1], NULL, worker_barrier_test, &bctx) == 0) {
        int main_rc = pthread_barrier_wait(&bctx.barrier);
        if (main_rc == PTHREAD_BARRIER_SERIAL_THREAD) {
            pthread_mutex_lock(&bctx.mutex);
            bctx.serial_thread_count++;
            pthread_mutex_unlock(&bctx.mutex);
        }
        pthread_join(tb[0], &brc0);
        pthread_join(tb[1], &brc1);
    }
    pthread_barrier_destroy(&bctx.barrier);
    pthread_mutex_destroy(&bctx.mutex);
    FLX_TEST_INT(&t, bctx.serial_thread_count, "==", 1,
                 "pthread_barrier_wait (3 threads synchronized, serial winner count=%d)",
                 bctx.serial_thread_count);

    /* 5. POSIX semaphore */
    sem_t sem;
    bool sem_ok = (sem_init(&sem, 0, 0) == 0 &&
                   pthread_create(&t1, NULL, worker_sem_test, &sem) == 0 &&
                   sem_wait(&sem) == 0);
    if (sem_ok) {
        pthread_join(t1, NULL);
    }
    sem_destroy(&sem);
    FLX_TEST_CHECK(&t, sem_ok, "sem_init / sem_post / sem_wait");

    /* 6. POSIX message queue */
#if defined(CONFIG_FELIX_POSIX_MQUEUE)
    struct mq_attr mq_attr = {
        .mq_maxmsg = 5,
        .mq_msgsize = 32,
        .mq_flags = 0,
        .mq_curmsgs = 0
    };
    mqd_t mq = mq_open("/test_q", O_CREAT | O_RDWR, 0644, &mq_attr);
    bool mq_ok = false;
    char rx_buf[32] = {0};
    if (mq != (mqd_t)-1) {
        const char *tx_msg = "FLX_MQ_MSG";
        if (mq_send(mq, tx_msg, strlen(tx_msg) + 1, 10) == 0) {
            unsigned int prio = 0;
            if (mq_receive(mq, rx_buf, sizeof(rx_buf), &prio) > 0) {
                mq_ok = (strcmp(rx_buf, tx_msg) == 0);
            }
        }
        mq_close(mq);
        mq_unlink("/test_q");
    }
    FLX_TEST_CHECK(&t, mq_ok, "mqueue (mq_open/send/receive/close/unlink msg='%s')", rx_buf);
#endif

    /* 7. POSIX timer (SIGEV_THREAD 20ms one-shot) */
#if defined(CONFIG_FELIX_POSIX_TIMER)
    sem_t timer_sem;
    sem_init(&timer_sem, 0, 0);
    struct sigevent sev = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = worker_timer_cb,
        .sigev_value.sival_ptr = &timer_sem,
        .sigev_notify_attributes = NULL
    };
    timer_t tmr;
    bool timer_ok = false;
    if (timer_create(CLOCK_MONOTONIC, &sev, &tmr) == 0) {
        struct itimerspec its = {
            .it_interval = { 0, 0 },
            .it_value = { 0, 20000000L } /* 20 ms */
        };
        if (timer_settime(tmr, 0, &its, NULL) == 0) {
            timer_ok = (sem_wait(&timer_sem) == 0);
        }
        timer_delete(tmr);
    }
    sem_destroy(&timer_sem);
    FLX_TEST_CHECK(&t, timer_ok, "timer_create / timer_settime (SIGEV_THREAD 20ms one-shot)");
#endif

    /* 8. pthread + Felix VFS coexistence (TLS PCB inheritance) */
    const char *test_path = "/tmp/posix_vfs_test.txt";
    void *vfs_ret = (void *)-1L;
    char buf[32] = {0};
    if (pthread_create(&t1, NULL, worker_vfs_test, (void *)test_path) == 0 &&
        pthread_join(t1, &vfs_ret) == 0 &&
        (intptr_t)vfs_ret == 0) {
        int fd = open(test_path, O_RDONLY);
        if (fd >= 0) {
            (void)read(fd, buf, sizeof(buf) - 1);
            close(fd);
        }
        unlink(test_path);
    }
    FLX_TEST_STR(&t, buf, "==", "POSIX_PTHREAD_VFS_OK",
                 "pthread VFS I/O coexistence ('%s')", buf);

    return flx_test_end(&t);
}
#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_POSIX */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_posix)
int flx_fs_test_cmd_posix(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_POSIX */
