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

#include "../oc_suite_common.hpp"

namespace ams::ldr::oc::pcv {

typedef struct cvb_coefficients {
    s32 c0 = 0;
    s32 c1 = 0;
    s32 c2 = 0;
    s32 c3 = 0;
    s32 c4 = 0;
    s32 c5 = 0;
} cvb_coefficients;

typedef struct cpu_freq_cvb_table_t {
    u64 freq;
    cvb_coefficients cvb_dfll_param;
    cvb_coefficients cvb_pll_param;
} cpu_freq_cvb_table_t;
static_assert(sizeof(cpu_freq_cvb_table_t) == 0x38);

typedef struct gpu_cvb_pll_table_t {
    u64 freq;
    cvb_coefficients cvb_dfll_param;
    cvb_coefficients cvb_pll_param;
} gpu_cvb_pll_table_t;
static_assert(sizeof(gpu_cvb_pll_table_t) == 0x38);

typedef struct emc_dvb_dvfs_table_t {
    u64 freq;
    s32 volt[4] = {0};
} emc_dvb_dvfs_table_t;

typedef struct __attribute__((packed)) div_nmp {
    u8 divn_shift;
    u8 divn_width;
    u8 divm_shift;
    u8 divm_width;
    u8 divp_shift;
    u8 divp_width;
    u8 override_divn_shift;
    u8 override_divm_shift;
    u8 override_divp_shift;
} div_nmp;

typedef struct __attribute__((packed)) clk_pll_param {
    u32 freq;
    u32 input_min;
    u32 input_max;
    u32 cf_min;
    u32 cf_max;
    u32 vco_min;
    u32 vco_max;
    s32 lock_delay;
    u32 fixed_rate;
    u32 unk_0;
    struct div_nmp *div_nmp;
    u32 unk_1[4];
    void (*unk_fn)(u64* unk_struct); // set_defaults?
} clk_pll_param;

typedef struct __attribute__((packed)) dvfs_rail {
    u32 id;
    u32 unk_0[5];
    u32 freq;
    u32 unk_1[8];
    u32 unk_flag;
    u32 min_mv;
    u32 step_mv;
    u32 max_mv;
    u32 unk_2[11];
} dvfs_rail;

constexpr u32 CpuClkOSLimit   = 1785'000;

constexpr u32 MemClkOSLimit   = 1600'000;

#define R_SKIP() R_SUCCEED()

Result MemFreqPllmLimit(u32* ptr);

template<typename Table>
Result MemMtcTableClone(Table* des, Table* src) {
    constexpr u32 mtc_magic = 0x5F43544D;
    R_UNLESS(src->rev == mtc_magic, ldr::ResultInvalidMtcMagic());

    // Skip params from dvfs_ver to clock_src;
    for (size_t offset = offsetof(Table, clk_src_emc); offset < sizeof(Table); offset += sizeof(u32)) {
        u32* src_ent = reinterpret_cast<u32 *>(reinterpret_cast<size_t>(src) + offset);
        u32* des_ent = reinterpret_cast<u32 *>(reinterpret_cast<size_t>(des) + offset);
        u32 src_val = *src_ent;

        constexpr u32 placeholder_val = UINT32_MAX;
        if (src_val != placeholder_val) {
            PatchOffset(des_ent, src_val);
        }
    }

    R_SUCCEED();
};

namespace erista {
    void Patch(uintptr_t mapped_nso, size_t nso_size);
}

namespace mariko {
    void Patch(uintptr_t mapped_nso, size_t nso_size);
}

void SafetyCheck();
void Patch(uintptr_t mapped_nso, size_t nso_size);

}
