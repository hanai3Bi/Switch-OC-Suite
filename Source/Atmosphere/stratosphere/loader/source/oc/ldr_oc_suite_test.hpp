#ifdef OC_TEST
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

#define R_SUCCEEDED(arg)  (arg == ResultSuccess())
#define LOGGING(fmt, ...) { printf(fmt "\n\tin %s\n", ##__VA_ARGS__, __PRETTY_FUNCTION__); }
#define AMS_ABORT()       { fprintf(stderr, "Failed!\n"); exit(-1); }

inline Result ResultSuccess() { return 0; }

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
    .eristaEmcVolt     = 1250,
    .eristaMtc = reinterpret_cast<EristaMtcTable *>(const_cast<u8 *>(EmptyMtcTable)),
    };
}
#endif