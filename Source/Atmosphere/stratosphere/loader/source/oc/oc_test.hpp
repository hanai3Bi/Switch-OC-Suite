/*
 * Copyright (C) Switch-OC-Suite
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef ATMOSPHERE_IS_STRATOSPHERE
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int      Result;

#define R_SUCCEEDED(arg)   (arg == 0)
#define R_FAILED(arg)      (arg != 0)
#define LOGGING(fmt, ...)  { printf(fmt "\n", ##__VA_ARGS__); }
#define CRASH(msg, ...)    { fprintf(stderr, "%s\nFailed in %s!\n", msg, __PRETTY_FUNCTION__); exit(-1); }
#define R_SUCCEED()        { return 0; }
#define R_THROW(err)       { return err; }
#define R_TRY(expr)        { Result _rc = (expr); if (R_FAILED(_rc)) { return _rc; } }
#define R_UNLESS(expr, rc) { if (!(expr)) { return rc; } }

#define R_DEFINE_ERROR_RESULT(name, rc)        \
    inline Result Result##name() { return rc; }

#define HEXDUMP(ptr, len)                                           \
    {                                                               \
        const uint8_t* p = reinterpret_cast<const uint8_t *>(ptr);  \
        size_t i, j;                                                \
        for (i = 0; i < len; i += 16) {                             \
            printf("%06zx: ", i);                                   \
            for (j = 0; j < 16 && i + j < len; j++)                 \
                printf("%02x ", p[i + j]);                          \
            for (; j < 16; j++)                                     \
                printf("   ");                                      \
            for (j = 0; j < 16 && i + j < len; j++)                 \
                printf("%c", isprint(p[i + j]) ? p[i + j] : '.');   \
            printf("\n");                                           \
        }                                                           \
    }                                                               \

typedef struct UnitTest {
    using Func = Result(*)();

    const char* description;
    Func        fun = nullptr;

    void Test() {
        Result res = fun();
        if (R_FAILED(res)) {
            CRASH(description);
        }
    }
} UnitTest;

#endif