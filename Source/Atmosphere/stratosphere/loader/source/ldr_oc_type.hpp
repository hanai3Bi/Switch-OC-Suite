#include "mtc_timing_table.hpp"

constexpr u32 CpuClkOSLimit   = 1785'000;
constexpr u32 CpuClkOfficial  = 1963'500;
constexpr u32 CpuVoltOfficial = 1120;
constexpr u32 GpuClkOfficial  = 1267'200;
constexpr u32 MemClkOSLimit   = 1600'000;
constexpr u32 MemClkOSAlt     = 1331'200;
constexpr u32 MemClkOSClampDn = 1065'600;

#define INVALID_MTC_TABLE UINT32_MAX

typedef struct {
    u8  magic[4]    = {'C', 'U', 'S', 'T'};
    u32 cpuMaxClock = CpuClkOfficial;
    u32 cpuMaxVolt  = CpuVoltOfficial;
    u32 gpuMaxClock = GpuClkOfficial;
    u32 emcMaxClock = 1862'400;
    MarikoMtcTable mtcTable = { INVALID_MTC_TABLE };
} CustomizeTable;

inline void PatchOffset(uintptr_t offset, u32 value) {
    *(reinterpret_cast<u32 *>(offset)) = value;
}

inline void PatchOffset(u32* offset, u32 value) {
    *(offset) = value;
}

#define ResultFailure() -1

namespace pcv {
    typedef struct {
        s32 c0 = 0;
        s32 c1 = 0;
        s32 c2 = 0;
        s32 c3 = 0;
        s32 c4 = 0;
        s32 c5 = 0;
    } cvb_coefficients;

    typedef struct {
        u64 freq;
        cvb_coefficients cvb_dfll_param;
        cvb_coefficients cvb_pll_param;  // only c0 is reserved
    } cpu_freq_cvb_table_t;

    typedef struct {
        u64 freq;
        cvb_coefficients cvb_dfll_param; // empty, dfll clock source not selected
        cvb_coefficients cvb_pll_param;
    } gpu_cvb_pll_table_t;

    typedef struct {
        u64 freq;
        s32 volt[4] = {0};
    } emc_dvb_dvfs_table_t;
}

namespace ptm {
    typedef struct {
        u32 conf_id;
        u32 cpu_freq_1;
        u32 cpu_freq_2;
        u32 gpu_freq_1;
        u32 gpu_freq_2;
        u32 emc_freq_1;
        u32 emc_freq_2;
        u32 padding;
    } perf_conf_entry;
}