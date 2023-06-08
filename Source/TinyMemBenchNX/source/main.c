/*
 * Copyright © 2011 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * pthread fork by sun409 (https://github.com/sun409/tinymembench-pthread)
 *
 * Switch port by Kazushi and built with libnx.
 */

// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

// Multi-thread support
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

#define __ASM_OPT_H__
#define SIZE             (32 * 1024 * 1024)
#define BLOCKSIZE        2048
#ifndef MAXREPEATS
# define MAXREPEATS      10
#endif
#ifndef LATBENCH_COUNT
# define LATBENCH_COUNT  10000000
#endif

#define ALIGN_PADDING    0x100000
#define CACHE_LINE_SIZE  128

#include "aarch64-asm.h"
#include <switch.h>

PadState pad;

struct f_data
{
    void (*func)(int64_t *, int64_t *, int);
    int64_t *arg1;
    int64_t *arg2;
    int      arg3;
};

pthread_cond_t p_ready, p_start;
pthread_mutex_t p_lock;
pthread_t *p_worker = NULL;
struct f_data *worker_data = NULL;
int p_worker_not_ready, p_workers_ready;

void *thread_func(void *data)
{
    struct f_data *data_ptr = data;

    pthread_mutex_lock(&p_lock);
    p_worker_not_ready--;

    if (!p_worker_not_ready)
        pthread_cond_signal(&p_ready);

    while (p_workers_ready != 1)
        pthread_cond_wait(&p_start, &p_lock);

    pthread_mutex_unlock(&p_lock);

    (data_ptr->func)(data_ptr->arg1, data_ptr->arg2, data_ptr->arg3);

    pthread_exit(NULL);
}

static void parallel_run(void)
{
    pthread_mutex_lock(&p_lock);
    p_workers_ready = 1;
    pthread_mutex_unlock(&p_lock);
    pthread_cond_broadcast(&p_start);
}

static void parallel_init(int threads)
{
    pthread_attr_t attr;

    pthread_cond_init(&p_ready, NULL);
    pthread_cond_init(&p_start, NULL);
    pthread_mutex_init(&p_lock, NULL);
    p_worker_not_ready = threads;
    p_workers_ready = 0;
    pthread_attr_init(&attr);

    if (!p_worker || !worker_data)
    {
        p_worker = (pthread_t *)malloc(threads * sizeof(pthread_t));
        worker_data = (struct f_data *)malloc(threads * sizeof(struct f_data));
    }

    for (int i = 0; i < threads; i++)
    {
        pthread_create(p_worker + i, &attr, thread_func, worker_data + i);
    }

    pthread_mutex_lock(&p_lock);
    while (p_worker_not_ready != 0)
        pthread_cond_wait(&p_ready, &p_lock);

    pthread_mutex_unlock(&p_lock);
}

typedef struct
{
    const char *description;
    int use_tmpbuf;
    void (*f)(int64_t *, int64_t *, int);
} bench_info;

static char *align_up(char *ptr, int align)
{
    return (char *)(((uintptr_t)ptr + align - 1) & ~(uintptr_t)(align - 1));
}

void aligned_block_copy(int64_t * __restrict dst_,
                        int64_t * __restrict src,
                        int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    while ((size -= 64) >= 0)
    {
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
    }
}

void aligned_block_copy_backwards(int64_t * __restrict dst_,
                                  int64_t * __restrict src,
                                  int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    src += size / 8 - 1;
    dst += size / 8 - 1;
    while ((size -= 64) >= 0)
    {
        t1 = *src--;
        t2 = *src--;
        t3 = *src--;
        t4 = *src--;
        *dst-- = t1;
        *dst-- = t2;
        *dst-- = t3;
        *dst-- = t4;
        t1 = *src--;
        t2 = *src--;
        t3 = *src--;
        t4 = *src--;
        *dst-- = t1;
        *dst-- = t2;
        *dst-- = t3;
        *dst-- = t4;
    }
}

void aligned_block_copy_backwards_bs32(int64_t * __restrict dst_,
                                       int64_t * __restrict src,
                                       int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    src += size / 8 - 8;
    dst += size / 8 - 8;
    while ((size -= 64) >= 0)
    {
        t1 = src[4];
        t2 = src[5];
        t3 = src[6];
        t4 = src[7];
        dst[4] = t1;
        dst[5] = t2;
        dst[6] = t3;
        dst[7] = t4;
        t1 = src[0];
        t2 = src[1];
        t3 = src[2];
        t4 = src[3];
        dst[0] = t1;
        dst[1] = t2;
        dst[2] = t3;
        dst[3] = t4;
        src -= 8;
        dst -= 8;
    }
}

void aligned_block_copy_backwards_bs64(int64_t * __restrict dst_,
                                       int64_t * __restrict src,
                                       int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    src += size / 8 - 8;
    dst += size / 8 - 8;
    while ((size -= 64) >= 0)
    {
        t1 = src[0];
        t2 = src[1];
        t3 = src[2];
        t4 = src[3];
        dst[0] = t1;
        dst[1] = t2;
        dst[2] = t3;
        dst[3] = t4;
        t1 = src[4];
        t2 = src[5];
        t3 = src[6];
        t4 = src[7];
        dst[4] = t1;
        dst[5] = t2;
        dst[6] = t3;
        dst[7] = t4;
        src -= 8;
        dst -= 8;
    }
}

void aligned_block_copy_pf32(int64_t * __restrict dst_,
                             int64_t * __restrict src,
                             int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    while ((size -= 64) >= 0)
    {
        __builtin_prefetch(src + 32, 0, 0);
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
        __builtin_prefetch(src + 32, 0, 0);
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
    }
}

void aligned_block_copy_pf64(int64_t * __restrict dst_,
                             int64_t * __restrict src,
                             int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t t1, t2, t3, t4;
    while ((size -= 64) >= 0)
    {
        __builtin_prefetch(src + 32, 0, 0);
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
        t1 = *src++;
        t2 = *src++;
        t3 = *src++;
        t4 = *src++;
        *dst++ = t1;
        *dst++ = t2;
        *dst++ = t3;
        *dst++ = t4;
    }
}

void aligned_block_fetch(int64_t * __restrict dst,
                         int64_t * __restrict src_,
                         int                  size)
{
    volatile int64_t *src = src_;
    while ((size -= 64) >= 0)
    {
        *src++;
        *src++;
        *src++;
        *src++;
        *src++;
        *src++;
        *src++;
        *src++;
    }
}

void aligned_block_fill(int64_t * __restrict dst_,
                        int64_t * __restrict src,
                        int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t data = *src;
    while ((size -= 64) >= 0)
    {
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
        *dst++ = data;
    }
}

void aligned_block_fill_shuffle16(int64_t * __restrict dst_,
                                  int64_t * __restrict src,
                                  int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t data = *src;
    while ((size -= 64) >= 0)
    {
        dst[0 + 0] = data;
        dst[1 + 0] = data;
        dst[1 + 2] = data;
        dst[0 + 2] = data;
        dst[1 + 4] = data;
        dst[0 + 4] = data;
        dst[0 + 6] = data;
        dst[1 + 6] = data;
        dst += 8;
    }
}

void aligned_block_fill_shuffle32(int64_t * __restrict dst_,
                                  int64_t * __restrict src,
                                  int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t data = *src;
    while ((size -= 64) >= 0)
    {
        dst[3 + 0] = data;
        dst[0 + 0] = data;
        dst[2 + 0] = data;
        dst[1 + 0] = data;
        dst[3 + 4] = data;
        dst[0 + 4] = data;
        dst[2 + 4] = data;
        dst[1 + 4] = data;
        dst += 8;
    }
}

void aligned_block_fill_shuffle64(int64_t * __restrict dst_,
                                  int64_t * __restrict src,
                                  int                  size)
{
    volatile int64_t *dst = dst_;
    int64_t data = *src;
    while ((size -= 64) >= 0)
    {
        dst[5] = data;
        dst[2] = data;
        dst[7] = data;
        dst[6] = data;
        dst[1] = data;
        dst[3] = data;
        dst[0] = data;
        dst[4] = data;
        dst += 8;
    }
}

double gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) / 1000000.;
}

static double bandwidth_bench_helper(int threads,
                                     int64_t *dstbuf, int64_t *srcbuf,
                                     int64_t *tmpbuf,
                                     int size, int blocksize,
                                     const char *indent_prefix,
                                     int use_tmpbuf,
                                     void (*f)(int64_t *, int64_t *, int),
                                     const char *description)
{
    int i, j, loopcount, innerloopcount, n;
    double t, t1, t2;
    double speed, maxspeed;
    double s, s0, s1, s2;

    /* do up to MAXREPEATS measurements */
    s = s0 = s1 = s2 = 0.;
    maxspeed = 0.;
    for (n = 0; n < MAXREPEATS; n++)
    {
        parallel_init(threads);
        for (int pt = 0; pt < threads; pt++)
        {
            (worker_data + pt)->func = f;
            (worker_data + pt)->arg1 = dstbuf + size * pt / sizeof(int64_t);
            (worker_data + pt)->arg2 = srcbuf + size * pt / sizeof(int64_t);
            (worker_data + pt)->arg3 = size;
        }
        parallel_run();
        for (int pt = 0; pt < threads; pt++)
            pthread_join(p_worker[pt], NULL);

        loopcount = 0;
        innerloopcount = 1;
        t = 0.;
        do
        {
            loopcount += innerloopcount;
            if (use_tmpbuf)
            {
                for (i = 0; i < innerloopcount; i++)
                {
                    t1 = gettime();
                    for (j = 0; j < size; j += blocksize)
                    {
                        f(tmpbuf, srcbuf + j / sizeof(int64_t), blocksize);
                        f(dstbuf + j / sizeof(int64_t), tmpbuf, blocksize);
                    }
                    t2 = gettime();
                    t += t2 - t1;
                }
            }
            else
            {
                for (i = 0; i < innerloopcount; i++)
                {
                    parallel_init(threads);
                    for (int pt = 0; pt < threads; ++pt)
                    {
                        (worker_data + pt)->func = f;
                        (worker_data + pt)->arg1 = dstbuf + size * pt / sizeof(int64_t);
                        (worker_data + pt)->arg2 = srcbuf + size * pt / sizeof(int64_t);
                        (worker_data + pt)->arg3 = size;
                    }

                    t1 = gettime();
                    parallel_run();
                    for (int pt = 0; pt < threads; ++pt)
                        pthread_join(p_worker[pt], NULL);
                    t2 = gettime();
                    t += t2 - t1;
                }
            }
            innerloopcount *= 2;
        } while (t < 0.5);
        speed = (double)size * (use_tmpbuf ? 1 : threads) * loopcount / t / 1000000.;

        s0 += 1.;
        s1 += speed;
        s2 += speed * speed;

        if (speed > maxspeed)
            maxspeed = speed;

        if (s0 > 2.)
        {
            s = sqrt((s0 * s2 - s1 * s1) / (s0 * (s0 - 1)));
            if (s < maxspeed / 1000.)
                break;
        }
    }

    if (maxspeed > 0 && s / maxspeed * 100. >= 0.1)
    {
        printf("%s%-40s : %8.1f MB/s (%.1f%%)\n", indent_prefix, description,
                                               maxspeed, s / maxspeed * 100.);
    }
    else
    {
        printf("%s%-40s : %8.1f MB/s\n", indent_prefix, description, maxspeed);
    }

    consoleUpdate(NULL);
    return maxspeed;
}

void bandwidth_bench(int threads,
                     int64_t *dstbuf, int64_t *srcbuf, int64_t *tmpbuf,
                     int size, int blocksize, const char *indent_prefix,
                     bench_info *bi)
{
    while (bi->f)
    {
        bandwidth_bench_helper(threads,
                               dstbuf, srcbuf, tmpbuf, size, blocksize,
                               indent_prefix, bi->use_tmpbuf,
                               bi->f,
                               bi->description);
        bi++;
    }
}

void memcpy_wrapper(int64_t *dst, int64_t *src, int size)
{
    memcpy(dst, src, size);
}

void memset_wrapper(int64_t *dst, int64_t *src, int size)
{
    memset(dst, src[0], size);
}

static bench_info aarch64_neon[] =
{
    { "NEON LDP (READ)", 0, aligned_block_read_ldp_q_aarch64 },
    { "NEON LDP/STP copy (COPY)", 0, aligned_block_copy_ldpstp_q_aarch64 },
    { "NEON LDP/STP copy pldl2strm (32B step)", 0, aligned_block_copy_ldpstp_q_pf32_l2strm_aarch64 },
    { "NEON LDP/STP copy pldl2strm (64B step)", 0, aligned_block_copy_ldpstp_q_pf64_l2strm_aarch64 },
    { "NEON LDP/STP copy pldl1keep (32B step)", 0, aligned_block_copy_ldpstp_q_pf32_l1keep_aarch64 },
    { "NEON LDP/STP copy pldl1keep (64B step)", 0, aligned_block_copy_ldpstp_q_pf64_l1keep_aarch64 },
    { "NEON LD1/ST1 copy", 0, aligned_block_copy_ld1st1_aarch64 },
    { "NEON STP fill (WRITE)", 0, aligned_block_fill_stp_q_aarch64 },
    { "NEON STNP fill", 0, aligned_block_fill_stnp_q_aarch64 },
    { "ARM LDP", 0, aligned_block_read_ldp_x_aarch64 },
    { "ARM LDP/STP copy", 0, aligned_block_copy_ldpstp_x_aarch64 },
    { "ARM STP fill", 0, aligned_block_fill_stp_x_aarch64 },
    { "ARM STNP fill", 0, aligned_block_fill_stnp_x_aarch64 },
    { NULL, 0, NULL }
};

bench_info *get_asm_benchmarks(void)
{
    return aarch64_neon;
}

static bench_info c_benchmarks[] =
{
    { "C copy backwards", 0, aligned_block_copy_backwards },
    { "C copy backwards (32B blocks)", 0, aligned_block_copy_backwards_bs32 },
    { "C copy backwards (64B blocks)", 0, aligned_block_copy_backwards_bs64 },
    { "C copy", 0, aligned_block_copy },
    { "C copy prefetched (32B step)", 0, aligned_block_copy_pf32 },
    { "C copy prefetched (64B step)", 0, aligned_block_copy_pf64 },
    // { "C 2-pass copy", 1, aligned_block_copy },
    // { "C 2-pass copy prefetched (32B step)", 1, aligned_block_copy_pf32 },
    // { "C 2-pass copy prefetched (64B step)", 1, aligned_block_copy_pf64 },
    { "C fetch", 0, aligned_block_fetch },
    { "C fill", 0, aligned_block_fill },
    { "C fill (shuffle within 16B blocks)", 0, aligned_block_fill_shuffle16 },
    { "C fill (shuffle within 32B blocks)", 0, aligned_block_fill_shuffle32 },
    { "C fill (shuffle within 64B blocks)", 0, aligned_block_fill_shuffle64 },
    { NULL, 0, NULL }
};

static bench_info libc_benchmarks[] =
{
    { "standard memcpy", 0, memcpy_wrapper },
    { "standard memset", 0, memset_wrapper },
    { NULL, 0, NULL }
};

void *alloc_four_nonaliased_buffers(void **buf1_, int size1,
                                    void **buf2_, int size2,
                                    void **buf3_, int size3,
                                    void **buf4_, int size4)
{
    char **buf1 = (char **)buf1_, **buf2 = (char **)buf2_;
    char **buf3 = (char **)buf3_, **buf4 = (char **)buf4_;
    int antialias_pattern_mask = (ALIGN_PADDING - 1) & ~(CACHE_LINE_SIZE - 1);
    char *buf, *ptr;

    if (!buf1 || size1 < 0)
        size1 = 0;
    if (!buf2 || size2 < 0)
        size2 = 0;
    if (!buf3 || size3 < 0)
        size3 = 0;
    if (!buf4 || size4 < 0)
        size4 = 0;

    ptr = buf = 
        (char *)malloc(size1 + size2 + size3 + size4 + 9 * ALIGN_PADDING);
    memset(buf, 0xCC, size1 + size2 + size3 + size4 + 9 * ALIGN_PADDING);

    ptr = align_up(ptr, ALIGN_PADDING);
    if (buf1)
    {
        *buf1 = ptr + (0xAAAAAAAA & antialias_pattern_mask);
        ptr = align_up(*buf1 + size1, ALIGN_PADDING);
    }
    if (buf2)
    {
        *buf2 = ptr + (0x55555555 & antialias_pattern_mask);
        ptr = align_up(*buf2 + size2, ALIGN_PADDING);
    }
    if (buf3)
    {
        *buf3 = ptr + (0xCCCCCCCC & antialias_pattern_mask);
        ptr = align_up(*buf3 + size3, ALIGN_PADDING);
    }
    if (buf4)
    {
        *buf4 = ptr + (0x33333333 & antialias_pattern_mask);
    }

    return buf;
}

#pragma GCC diagnostic push
static void __attribute__((noinline)) random_read_test(char *zerobuffer,
                                                       int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v;

    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    static volatile uint32_t dummy;

    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v = (seed >> 16) & 0xFF;                \
        seed = seed * 1103515245 + 12345;       \
        v |= (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v |= seed & 0x7FFF0000;                 \
        seed |= zerobuffer[v & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}

static void __attribute__((noinline)) random_dual_read_test(char *zerobuffer,
                                                            int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v1, v2;

    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    static volatile uint32_t dummy;

    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v1 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v2 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v1 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v2 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v1 |= (seed >> 16) & 0xFF;              \
        v2 |= (seed >> 24);                     \
        v2 &= addrmask;                         \
        v1 ^= v2;                               \
        seed |= zerobuffer[v2];                 \
        seed += zerobuffer[v1 & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}
#pragma GCC diagnostic pop

static uint32_t rand32()
{
    static int seed = 0;
    uint32_t hi, lo;
    hi = (seed = seed * 1103515245 + 12345) >> 16;
    lo = (seed = seed * 1103515245 + 12345) >> 16;
    return (hi << 16) + lo;
}

int latency_bench(int size, int count, int use_hugepage, int quick)
{
    double t, t2, t_before, t_after, t_noaccess, t_noaccess2 = 0;
    double xs, xs1, xs2;
    double ys, ys1, ys2;
    double min_t, min_t2;
    int nbits, n;
    char *buffer, *buffer_alloc;
#if !defined(__linux__) || !defined(MADV_HUGEPAGE)
    if (use_hugepage)
        return 0;
    buffer_alloc = (char *)malloc(size + 4095);
    if (!buffer_alloc)
        return 0;
    buffer = (char *)(((uintptr_t)buffer_alloc + 4095) & ~(uintptr_t)4095);
#else
    if (posix_memalign((void **)&buffer_alloc, 4 * 1024 * 1024, size) != 0)
        return 0;
    buffer = buffer_alloc;
    if (use_hugepage && madvise(buffer, size, use_hugepage > 0 ?
                                MADV_HUGEPAGE : MADV_NOHUGEPAGE) != 0)
    {
        free(buffer_alloc);
        return 0;
    }
#endif
    memset(buffer, 0, size);

    for (n = 1; n <= MAXREPEATS; n++)
    {
        t_before = gettime();
        random_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess)
            t_noaccess = t_after - t_before;

        t_before = gettime();
        random_dual_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess2)
            t_noaccess2 = t_after - t_before;
    }

    printf("\nblock size : single random read / dual random read");
    if (use_hugepage > 0)
        printf(", [MADV_HUGEPAGE]\n");
    else if (use_hugepage < 0)
        printf(", [MADV_NOHUGEPAGE]\n");
    else
        printf("\n");

    consoleUpdate(NULL);

    int start = quick ? 20 : 10;
    for (nbits = start; (1 << nbits) <= size; nbits++)
    {
        int testsize = 1 << nbits;
        xs1 = xs2 = ys = ys1 = ys2 = 0;
        for (n = 1; n <= MAXREPEATS; n++)
        {
            int testoffs = (rand32() % (size / testsize)) * testsize;

            t_before = gettime();
            random_read_test(buffer + testoffs, count, nbits);
            t_after = gettime();
            t = t_after - t_before - t_noaccess;
            if (t < 0) t = 0;

            xs1 += t;
            xs2 += t * t;

            if (n == 1 || t < min_t)
                min_t = t;

            t_before = gettime();
            random_dual_read_test(buffer + testoffs, count, nbits);
            t_after = gettime();
            t2 = t_after - t_before - t_noaccess2;
            if (t2 < 0) t2 = 0;

            ys1 += t2;
            ys2 += t2 * t2;

            if (n == 1 || t2 < min_t2)
                min_t2 = t2;

            if (n > 2)
            {
                xs = sqrt((xs2 * n - xs1 * xs1) / (n * (n - 1)));
                ys = sqrt((ys2 * n - ys1 * ys1) / (n * (n - 1)));
                if (xs < min_t / 1000. && ys < min_t2 / 1000.)
                    break;
            }
        }
        printf("%10d : %6.1f ns          /  %6.1f ns \n", (1 << nbits),
            min_t * 1000000000. / count,  min_t2 * 1000000000. / count);

        consoleUpdate(NULL);
    }
    free(buffer_alloc);
    return 1;
}

void waitForKeyA() {
    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
            break; 
        else if(kDown)
        {
            consoleExit(NULL);
            exit(0);
        }

        consoleUpdate(NULL);
    }
}

void printClock()
{
    int res = 0;
    uint32_t cpu_hz = 0, mem_hz = 0;

    ClkrstSession clkrstSession;
    res = clkrstInitialize();
    if(R_FAILED(res)) {
        fatalThrow(res);
    }

    clkrstOpenSession(&clkrstSession, PcvModuleId_CpuBus, 3);
    clkrstGetClockRate(&clkrstSession, &cpu_hz);
    clkrstCloseSession(&clkrstSession);
    clkrstOpenSession(&clkrstSession, PcvModuleId_EMC, 3);
    clkrstGetClockRate(&clkrstSession, &mem_hz);
    clkrstCloseSession(&clkrstSession);
    clkrstExit();

    printf("== CPU: %u.%u MHz ==\n== MEM: %u.%u MHz ==\n",
        cpu_hz/1000000, cpu_hz/100000 - cpu_hz/1000000*10,
        mem_hz/1000000, mem_hz/100000 - mem_hz/1000000*10);
    consoleUpdate(NULL);
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    consoleInit(NULL);

    printf("TinyMemBenchNX v0.4.11\n\
(based on tinymembench-pthread, a multi-thread fork of simple benchmark for memory throughput and latency)\n\n");
    printf("Copyright (c) 2021-2022 KazushiMe\n");
    printf("Copyright (c) 2023 hanai3Bi\n");

    printf("\n");
    consoleUpdate(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    padInitializeDefault(&pad);

    int64_t *srcbuf, *dstbuf, *tmpbuf;
    void *poolbuf;
    size_t bufsize = SIZE;
    int threads = 0;

loop:
    printf("!!! Memory bandwidth heavily depends on CPU clock. !!!\n\n");
    printf("\
Press A to start quick test.\n\
Press X to start bandwidth test.\n\
Press Y to start latency test.\n\
Press any other key to exit.\n\n");
    consoleUpdate(NULL);

    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
        {
            threads = 3;
            goto quick;
            break;
        }
        else if (kDown & HidNpadButton_X)
        {
            threads = 3;
            goto bandwidth;
            break;
        }
        else if (kDown & HidNpadButton_Y)
        {
            threads = 3;
            goto latency;
            break;
        }
        else if (kDown)
        {
            consoleExit(NULL);
            exit(0);
        }
    }

quick:

    poolbuf = alloc_four_nonaliased_buffers((void **)&srcbuf, bufsize * threads,
                                            (void **)&dstbuf, bufsize * threads,
                                            (void **)&tmpbuf, BLOCKSIZE * threads,
                                            NULL, 0);

    printClock();
    printf("== Thread: %d ==\n", threads);
    consoleUpdate(NULL);

    bandwidth_bench(threads, dstbuf, srcbuf, tmpbuf, bufsize, BLOCKSIZE/2, " ", libc_benchmarks);
    
    free(poolbuf);

    int latbench_size = SIZE * 2, latbench_count = LATBENCH_COUNT;

    if (!latency_bench(latbench_size, latbench_count, -1, 1) ||
        !latency_bench(latbench_size, latbench_count, 1, 1))
    {
        latency_bench(latbench_size, latbench_count, 0, 1);
    }

    printf("\nPress A to continue, any other key to exit.\n\n");
    waitForKeyA();
    consoleClear();
    goto loop;

bandwidth:

    printf("==========================================================================\n");
    printf("== Memory bandwidth tests                                               ==\n");
    printf("==                                                                      ==\n");
    printf("== Note 1: 1MB = 1000000 bytes                                          ==\n");
    printf("== Note 2: Results for 'copy' tests show how many bytes can be          ==\n");
    printf("==         copied per second (adding together read and writen           ==\n");
    printf("==         bytes would have provided twice higher numbers)              ==\n");
    printf("== Note 3: 2-pass copy means that we are using a small temporary buffer ==\n");
    printf("==         to first fetch data into it, and only then write it to the   ==\n");
    printf("==         destination (source -> L1 cache, L1 cache -> destination)    ==\n");
    printf("== Note 4: If sample standard deviation exceeds 0.1%%, it is shown in    ==\n");
    printf("==         brackets                                                     ==\n");
    printf("==========================================================================\n\n");

    poolbuf = alloc_four_nonaliased_buffers((void **)&srcbuf, bufsize * threads,
                                            (void **)&dstbuf, bufsize * threads,
                                            (void **)&tmpbuf, BLOCKSIZE * threads,
                                            NULL, 0);

    printClock();
    printf("== Thread: %d ==\n", threads);
    consoleUpdate(NULL);

    bandwidth_bench(threads, dstbuf, srcbuf, tmpbuf, bufsize, BLOCKSIZE, " ", c_benchmarks);
    printf(" ---\n");

    bandwidth_bench(threads, dstbuf, srcbuf, tmpbuf, bufsize, BLOCKSIZE, " ", libc_benchmarks);
    bench_info *bi = get_asm_benchmarks();
    if (bi->f) {
        printf(" ---\n");
        bandwidth_bench(threads, dstbuf, srcbuf, tmpbuf, bufsize, BLOCKSIZE, " ", bi);
    }

    free(poolbuf);

    printf("\nPress A to continue, any other key to exit.\n\n");
    waitForKeyA();
    consoleClear();
    goto loop;

latency:

    latbench_size = SIZE * 2, latbench_count = LATBENCH_COUNT;

    printf("\n");
    printf("==========================================================================\n");
    printf("== Memory latency test                                                  ==\n");
    printf("==                                                                      ==\n");
    printf("== Average time is measured for random memory accesses in the buffers   ==\n");
    printf("== of different sizes. The larger is the buffer, the more significant   ==\n");
    printf("== are relative contributions of TLB, L1/L2 cache misses and SDRAM      ==\n");
    printf("== accesses. For extremely large buffer sizes we are expecting to see   ==\n");
    printf("== page table walk with several requests to SDRAM for almost every      ==\n");
    printf("== memory access (though 64MiB is not nearly large enough to experience ==\n");
    printf("== this effect to its fullest).                                         ==\n");
    printf("==                                                                      ==\n");
    printf("== Note 1: All the numbers are representing extra time, which needs to  ==\n");
    printf("==         be added to L1 cache latency. The cycle timings for L1 cache ==\n");
    printf("==         latency can be usually found in the processor documentation. ==\n");
    printf("== Note 2: Dual random read means that we are simultaneously performing ==\n");
    printf("==         two independent memory accesses at a time. In the case if    ==\n");
    printf("==         the memory subsystem can't handle multiple outstanding       ==\n");
    printf("==         requests, dual random read has the same timings as two       ==\n");
    printf("==         single reads performed one after another.                    ==\n");
    printf("==========================================================================\n\n");

    consoleUpdate(NULL);
    printClock();
    
    if (!latency_bench(latbench_size, latbench_count, -1, 0) ||
        !latency_bench(latbench_size, latbench_count, 1, 0))
    {
        latency_bench(latbench_size, latbench_count, 0, 0);
    }

    printf("\nPress A to continue, any other key to exit.\n\n");
    waitForKeyA();
    consoleClear();
    goto loop;

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}