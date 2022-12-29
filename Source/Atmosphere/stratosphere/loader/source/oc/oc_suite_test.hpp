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
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

#endif