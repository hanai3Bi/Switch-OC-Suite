#ifndef ATMOSPHERE_IS_STRATOSPHERE
#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int      Result;

#define R_SUCCEEDED(arg)   (arg == 0)
#define R_FAILED(arg)      (arg != 0)
#define LOGGING(fmt, ...)  { printf(fmt "\n\tin %s\n", ##__VA_ARGS__, __PRETTY_FUNCTION__); }
#define AMS_ABORT()        { fprintf(stderr, "Failed in %s!\n", __PRETTY_FUNCTION__); exit(-1); }
#define R_SUCCEED()        { return 0; }
#define R_THROW(err)       { return err; }
#define R_TRY(expr)        { Result _rc = (expr); if (R_FAILED(_rc)) { return _rc; } }
#define R_UNLESS(expr, rc) { if (!(expr)) { return rc; } }

#define R_DEFINE_ERROR_RESULT(name, rc)        \
    inline Result Result##name() { return rc; }

#include "ldr_oc_suite.hpp"

namespace ams::ldr::oc {
    volatile u8 EmptyMtcTable[0x1340] = { 'M', 'T', 'C', '_', };
    static const volatile CustomizeTable C = {
    .mtcConf = AUTO_ADJ_MARIKO_SAFE,
    .marikoCpuMaxClock = 2397000,
    .marikoCpuMaxVolt  = 1220,
    .marikoGpuMaxClock = 1305600,
    .marikoEmcMaxClock = 1996800,
    .eristaCpuOCEnable = 1,
    .eristaCpuMaxVolt  = 1300,
    .eristaEmcMaxClock = 1862400,
    .eristaEmcVolt     = 1200'000,
    .eristaMtc = reinterpret_cast<EristaMtcTable *>(const_cast<u8 *>(EmptyMtcTable)),
    };
}
#endif