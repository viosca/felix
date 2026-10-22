#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_STRESS)

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <flx_test.h>

#if defined(CONFIG_FELIX_FREERTOS_PLUS_POSIX)
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include "kernel/sched.h"

typedef struct {
    pthread_mutex_t mutex;
    volatile int counter;
} StressMutexCtx_t;

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_stress)
static void *worker_churn(void *arg) {
    uintptr_t val = (uintptr_t)arg;
    return (void *)(val + 100);
}

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_stress)
static void *worker_stress_mutex(void *arg) {
    StressMutexCtx_t *ctx = (StressMutexCtx_t *)arg;
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&ctx->mutex);
        ctx->counter++;
        pthread_mutex_unlock(&ctx->mutex);
        sched_yield();
    }
    return NULL;
}
#endif

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_stress)
int flx_fs_test_cmd_stress(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS Multitasking Concurrency & Stress Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

#if defined(CONFIG_FELIX_FREERTOS_PLUS_POSIX)
    /* 1. Rapid pthread creation and cleanup churn */
    bool churn_ok = true;
    for (uintptr_t i = 0; i < 10; i++) {
        pthread_t th;
        void *ret = NULL;
        if (pthread_create(&th, NULL, worker_churn, (void *)i) != 0 ||
            pthread_join(th, &ret) != 0 ||
            (uintptr_t)ret != (i + 100)) {
            churn_ok = false;
            break;
        }
        /* Allow the FreeRTOS idle task a timeslice to run garbage collection
         * (free task stack & TCB on xTasksWaitingTermination) */
        usleep(1000);
    }
    FLX_TEST_CHECK(&t, churn_ok, "rapid pthread churn (10 threads created & joined sequentially)");

    /* 2. 4-thread concurrent mutex contention */
    StressMutexCtx_t sctx = { .counter = 0 };
    pthread_mutex_init(&sctx.mutex, NULL);
    pthread_t threads[4];
    bool spawn_ok = true;
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], NULL, worker_stress_mutex, &sctx) != 0) {
            spawn_ok = false;
        }
    }
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
        usleep(1000);
    }
    pthread_mutex_destroy(&sctx.mutex);
    FLX_TEST_CHECK(&t, spawn_ok && sctx.counter == 400,
                   "4-thread concurrent mutex contention (counter=%d/400)", sctx.counter);

    /* 3. Message queue throughput */
#if defined(CONFIG_FELIX_POSIX_MQUEUE)
    struct mq_attr mq_attr = {
        .mq_maxmsg = 10,
        .mq_msgsize = 32,
        .mq_flags = 0,
        .mq_curmsgs = 0
    };
    mqd_t q = mq_open("/stress_mq", O_CREAT | O_RDWR, 0644, &mq_attr);
    bool mq_stress_ok = false;
    if (q != (mqd_t)-1) {
        int sent_count = 0;
        int recv_count = 0;
        for (int i = 0; i < 10; i++) {
            char mbuf[32];
            snprintf(mbuf, sizeof(mbuf), "MSG_%02d", i);
            if (mq_send(q, mbuf, strlen(mbuf) + 1, 0) == 0) {
                sent_count++;
            }
        }
        bool order_ok = true;
        for (int i = 0; i < sent_count; i++) {
            char rx[32] = {0};
            if (mq_receive(q, rx, sizeof(rx), NULL) > 0) {
                char expected[32];
                snprintf(expected, sizeof(expected), "MSG_%02d", i);
                if (strcmp(rx, expected) == 0) {
                    recv_count++;
                } else {
                    order_ok = false;
                }
            }
        }
        mq_close(q);
        mq_unlink("/stress_mq");
        mq_stress_ok = (sent_count == 10 && recv_count == 10 && order_ok);
    }
    FLX_TEST_CHECK(&t, mq_stress_ok, "mqueue FIFO throughput (10/10 messages in sequence)");
#endif

    /* 4. Monotonic timer consistency */
    bool timer_timing_ok = true;
    for (int i = 0; i < 3; i++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        usleep(10000); /* 10 ms */
        clock_gettime(CLOCK_MONOTONIC, &t1);
        long elapsed_us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;
        if (elapsed_us < 8000L || elapsed_us > 40000L) {
            timer_timing_ok = false;
        }
    }
    FLX_TEST_CHECK(&t, timer_timing_ok, "monotonic timer jitter across 3x 10ms intervals (within bounds)");

#else
    FLX_TEST_CHECK(&t, true, "POSIX multitasking disabled in Kconfig (skipped)");
#endif

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_STRESS */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_stress)
int flx_fs_test_cmd_stress(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_STRESS */
