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
#include "mtc_timing_table.hpp"

#define CUST_REV 2

#ifdef ATMOSPHERE_IS_STRATOSPHERE
#include <vapours/results/results_common.hpp>
#define LOGGING(fmt, ...) ((void)0)
#endif

#define CRASH() { AMS_ABORT(); __builtin_unreachable(); }

namespace ams::ldr {
    R_DEFINE_ERROR_RESULT(OutOfRange,               1000);
    R_DEFINE_ERROR_RESULT(InvalidMemPllmEntry,      1001);
    R_DEFINE_ERROR_RESULT(InvalidMtcMagic,          1002);
    R_DEFINE_ERROR_RESULT(InvalidMtcTable,          1003);
    R_DEFINE_ERROR_RESULT(InvalidDvbTable,          1004);
    R_DEFINE_ERROR_RESULT(InvalidCpuClockVddEntry,  1005);
    R_DEFINE_ERROR_RESULT(InvalidCpuDvfs,           1006);
    R_DEFINE_ERROR_RESULT(InvalidCpuMinVolt,        1007);
    R_DEFINE_ERROR_RESULT(InvalidGpuDvfs,           1008);
    R_DEFINE_ERROR_RESULT(InvalidGpuMaxClkPattern,  1009);
    R_DEFINE_ERROR_RESULT(InvalidGpuPllEntry,       1010);
}

namespace ams::ldr::oc {
    enum MtcConfig {
        AUTO_ADJ_MARIKO_SAFE = 0,
        AUTO_ADJ_MARIKO_4266 = 1,
        ENTIRE_TABLE_ERISTA  = 2,
        ENTIRE_TABLE_MARIKO  = 3,
    };

    typedef struct CustomizeTable {
        u8  cust[4] = {'C', 'U', 'S', 'T'};
        u16 custRev = CUST_REV;
        u16 mtcConf = AUTO_ADJ_MARIKO_SAFE;
        u32 marikoCpuMaxClock;
        u32 marikoCpuBoostClock;
        u32 marikoCpuMaxVolt;
        u32 marikoGpuMaxClock;
        u32 marikoEmcMaxClock;
        u32 eristaCpuOCEnable;
        u32 eristaCpuMaxVolt;
        u32 eristaEmcMaxClock;
        u32 eristaEmcVolt;
        union {
            EristaMtcTable* eristaMtc;
            MarikoMtcTable* marikoMtc;
        };
    } CustomizeTable;

    inline void PatchOffset(u32* offset, u32 value) { *(offset) = value; }

    namespace pcv {
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
            cvb_coefficients cvb_pll_param;  // only c0 is reserved
        } cpu_freq_cvb_table_t;

        typedef struct gpu_cvb_pll_table_t {
            u64 freq;
            cvb_coefficients cvb_dfll_param; // empty, dfll clock source not selected
            cvb_coefficients cvb_pll_param;
        } gpu_cvb_pll_table_t;

        typedef struct emc_dvb_dvfs_table_t {
            u64 freq;
            s32 volt[4] = {0};
        } emc_dvb_dvfs_table_t;

        typedef struct clk_pll_param {
            u32 max_0;
            u32 unk[5];
            u32 max_1;
        } clk_pll_param;

        void Patch(uintptr_t mapped_nso, size_t nso_size);
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

        void Patch(uintptr_t mapped_nso, size_t nso_size);
    }
}