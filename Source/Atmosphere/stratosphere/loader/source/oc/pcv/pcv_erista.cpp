/*
 * Copyright (C) Switch-OC-Suite
 *
 * Copyright (c) 2023 hanai3Bi
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

#include "pcv.hpp"
#include "../mtc_timing_value.hpp"

namespace ams::ldr::oc::pcv::erista {

Result CpuVoltRange(u32* ptr) {
    u32 min_volt_got = *(ptr - 1);
    for (const auto& mv : CpuMinVolts) {
        if (min_volt_got != mv)
            continue;

        if (!C.eristaCpuMaxVolt)
            R_SKIP();

        PATCH_OFFSET(ptr, C.eristaCpuMaxVolt);
        R_SUCCEED();
    }
    R_THROW(ldr::ResultInvalidCpuMinVolt());
}

Result GpuFreqMaxAsm(u32* ptr32) {
    // Check if both two instructions match the pattern
    u32 ins1 = *ptr32, ins2 = *(ptr32 + 1);
    if (!(asm_compare_no_rd(ins1, asm_pattern[0]) && asm_compare_no_rd(ins2, asm_pattern[1])))
        R_THROW(ldr::ResultInvalidGpuFreqMaxPattern());

    // Both instructions should operate on the same register
    u8 rd = asm_get_rd(ins1);
    if (rd != asm_get_rd(ins2))
        R_THROW(ldr::ResultInvalidGpuFreqMaxPattern());

    u32 max_clock = GetDvfsTableLastEntry(C.eristaGpuDvfsTable)->freq;
           
    u32 asm_patch[2] = {
        asm_set_rd(asm_set_imm16(asm_pattern[0], max_clock), rd),
        asm_set_rd(asm_set_imm16(asm_pattern[1], max_clock >> 16), rd)
    };
    PATCH_OFFSET(ptr32, asm_patch[0]);
    PATCH_OFFSET(ptr32 + 1, asm_patch[1]);

    R_SUCCEED();
}

Result GpuFreqPllLimit(u32* ptr) {
    clk_pll_param* entry = reinterpret_cast<clk_pll_param *>(ptr);

    // All zero except for freq
    for (size_t i = 1; i < sizeof(clk_pll_param) / sizeof(u32); i++) {
        R_UNLESS(*(ptr + i) == 0, ldr::ResultInvalidGpuPllEntry());
    }

    // Double the max clk simply
    u32 max_clk = entry->freq * 2;
    entry->freq = max_clk;
    R_SUCCEED();
}

void MemMtcTableAutoAdjust(EristaMtcTable* table) {
    if (C.mtcConf != AUTO_ADJ_ALL)
    	return;

    #define WRITE_PARAM_ALL_REG(TABLE, PARAM, VALUE) \
    TABLE->burst_regs.PARAM = VALUE;                 \
    TABLE->shadow_regs_ca_train.PARAM = VALUE;       \
    TABLE->shadow_regs_quse_train.PARAM = VALUE;     \
    TABLE->shadow_regs_rdwr_train.PARAM = VALUE;
 
    #define GET_CYCLE_CEIL(PARAM)   u32(CEIL(double(PARAM) / tCK_avg))

    WRITE_PARAM_ALL_REG(table, emc_rc,                  GET_CYCLE_CEIL(tRC));
    WRITE_PARAM_ALL_REG(table, emc_rfc,                 GET_CYCLE_CEIL(tRFCab));
    WRITE_PARAM_ALL_REG(table, emc_rfcpb,               GET_CYCLE_CEIL(tRFCpb));
    WRITE_PARAM_ALL_REG(table, emc_ras,                 GET_CYCLE_CEIL(tRAS));
    WRITE_PARAM_ALL_REG(table, emc_rp,                  GET_CYCLE_CEIL(tRPpb));
    WRITE_PARAM_ALL_REG(table, emc_r2w,                 R2W);
    WRITE_PARAM_ALL_REG(table, emc_w2r,                 W2R);
    WRITE_PARAM_ALL_REG(table, emc_r2p,                 GET_CYCLE_CEIL(tRTP));
    WRITE_PARAM_ALL_REG(table, emc_w2p,                 WTP);
    WRITE_PARAM_ALL_REG(table, emc_rd_rcd,              GET_CYCLE_CEIL(tRCD));
    WRITE_PARAM_ALL_REG(table, emc_wr_rcd,              GET_CYCLE_CEIL(tRCD));
    WRITE_PARAM_ALL_REG(table, emc_rrd,                 GET_CYCLE_CEIL(tRRD));
    WRITE_PARAM_ALL_REG(table, emc_refresh,             REFRESH);
    WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
    WRITE_PARAM_ALL_REG(table, emc_pdex2wr,             GET_CYCLE_CEIL(tXP));
    WRITE_PARAM_ALL_REG(table, emc_pdex2rd,             GET_CYCLE_CEIL(tXP));
    WRITE_PARAM_ALL_REG(table, emc_pchg2pden,           GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_act2pden,            GET_CYCLE_CEIL(tMRWCKEL));
    WRITE_PARAM_ALL_REG(table, emc_ar2pden,             GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_rw2pden,             WTPDEN);
    WRITE_PARAM_ALL_REG(table, emc_cke2pden,            GET_CYCLE_CEIL(tCKELCS));
    WRITE_PARAM_ALL_REG(table, emc_pdex2cke,            GET_CYCLE_CEIL(tCSCKEH));
    WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,            GET_CYCLE_CEIL(tPDEX2MRR));
    WRITE_PARAM_ALL_REG(table, emc_txsr,                MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
    WRITE_PARAM_ALL_REG(table, emc_txsrdll,             MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
    WRITE_PARAM_ALL_REG(table, emc_tcke,                GET_CYCLE_CEIL(tCKE));
    WRITE_PARAM_ALL_REG(table, emc_tckesr,              GET_CYCLE_CEIL(tSR));
    WRITE_PARAM_ALL_REG(table, emc_tpd,                 GET_CYCLE_CEIL(tCKE));
    WRITE_PARAM_ALL_REG(table, emc_tfaw,                GET_CYCLE_CEIL(tFAW));
    WRITE_PARAM_ALL_REG(table, emc_trpab,               GET_CYCLE_CEIL(tRPab));
    WRITE_PARAM_ALL_REG(table, emc_tclkstable,          GET_CYCLE_CEIL(tCKCKEH));
    WRITE_PARAM_ALL_REG(table, emc_tclkstop,            GET_CYCLE_CEIL(tCKE)+8);
    WRITE_PARAM_ALL_REG(table, emc_trefbw,              REFBW);

    #define WRITE_PARAM_BURST_MC_REG(TABLE, PARAM, VALUE)   TABLE->burst_mc_regs.PARAM = VALUE;
    
    constexpr u32 MC_ARB_DIV = 4;
    constexpr u32 MC_ARB_SFA = 2;
    table->burst_mc_regs.mc_emem_arb_timing_rcd     = CEIL(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV) - 2;
    table->burst_mc_regs.mc_emem_arb_timing_rp      = CEIL(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    table->burst_mc_regs.mc_emem_arb_timing_rc      = CEIL(GET_CYCLE_CEIL(tRC) / MC_ARB_DIV) - 1;
    table->burst_mc_regs.mc_emem_arb_timing_ras     = CEIL(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV) - 2;
    table->burst_mc_regs.mc_emem_arb_timing_faw     = CEIL(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1;
    table->burst_mc_regs.mc_emem_arb_timing_rrd     = CEIL(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1;
    table->burst_mc_regs.mc_emem_arb_timing_rap2pre = CEIL(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV);
    table->burst_mc_regs.mc_emem_arb_timing_wap2pre = CEIL(WTP / MC_ARB_DIV);
    //table->burst_mc_regs.mc_emem_arb_timing_r2r     = CEIL(table->burst_regs.emc_rext / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    //table->burst_mc_regs.mc_emem_arb_timing_w2w     = CEIL(table->burst_regs.emc_wext / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    table->burst_mc_regs.mc_emem_arb_timing_r2w     = CEIL(R2W / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    table->burst_mc_regs.mc_emem_arb_timing_w2r     = CEIL(W2R / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = CEIL(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV);
    //table->burst_mc_regs.mc_emem_arb_timing_ccdmw   = CEIL(tCCDMW / MC_ARB_DIV) -1 + MC_ARB_SFA;
}

void MemMtcTableCustomAdjust(EristaMtcTable* table) {
    if (C.mtcConf != CUSTOM_ADJ_ALL)
    	return;
       
    constexpr u32 MC_ARB_DIV = 4;
    constexpr u32 MC_ARB_SFA = 2;

    if (TIMING_PRESET_ONE) {
        WRITE_PARAM_ALL_REG(table, emc_rc,       GET_CYCLE_CEIL(tRC));
        WRITE_PARAM_ALL_REG(table, emc_ras,      GET_CYCLE_CEIL(tRAS));
        WRITE_PARAM_ALL_REG(table, emc_rp,       GET_CYCLE_CEIL(tRPpb));
        WRITE_PARAM_ALL_REG(table, emc_trpab,    GET_CYCLE_CEIL(tRPab));
        WRITE_PARAM_ALL_REG(table, emc_rd_rcd,   GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_wr_rcd,   GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_pdex2mrr, GET_CYCLE_CEIL(tPDEX2MRR));

        table->burst_mc_regs.mc_emem_arb_timing_rcd     = CEIL(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV - 2);
        table->burst_mc_regs.mc_emem_arb_timing_rc      = CEIL(GET_CYCLE_CEIL(tRC) / MC_ARB_DIV - 1);
        table->burst_mc_regs.mc_emem_arb_timing_rp      = CEIL(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV - 1 + MC_ARB_SFA);
        table->burst_mc_regs.mc_emem_arb_timing_ras     = CEIL(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV - 2);
    }

    if (TIMING_PRESET_TWO) {
        WRITE_PARAM_ALL_REG(table, emc_tfaw,     GET_CYCLE_CEIL(tFAW));
        WRITE_PARAM_ALL_REG(table, emc_rrd,      GET_CYCLE_CEIL(tRRD));

        table->burst_mc_regs.mc_emem_arb_timing_faw     = CEIL(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1;
        table->burst_mc_regs.mc_emem_arb_timing_rrd     = CEIL(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1;
    }

    if (TIMING_PRESET_THREE) {
        WRITE_PARAM_ALL_REG(table, emc_r2p,      GET_CYCLE_CEIL(tRTP));
        WRITE_PARAM_ALL_REG(table, emc_w2p,      WTP);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden,  WTPDEN);

        table->burst_mc_regs.mc_emem_arb_timing_rap2pre = CEIL(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV);
        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = CEIL(WTP / MC_ARB_DIV);
    }

    if (TIMING_PRESET_FOUR) {
        WRITE_PARAM_ALL_REG(table, emc_rfc,      GET_CYCLE_CEIL(tRFCab));
        WRITE_PARAM_ALL_REG(table, emc_rfcpb,    GET_CYCLE_CEIL(tRFCpb));
        WRITE_PARAM_ALL_REG(table, emc_txsr,     MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
        WRITE_PARAM_ALL_REG(table, emc_txsrdll,  MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));

        table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = CEIL(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV);
    }

    if (TIMING_PRESET_FIVE) {
        WRITE_PARAM_ALL_REG(table, emc_w2r, W2R);

        table->burst_mc_regs.mc_emem_arb_timing_w2r     = CEIL(W2R / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    }

    if (TIMING_PRESET_SIX) {
        WRITE_PARAM_ALL_REG(table, emc_refresh,  REFRESH);
        WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
        WRITE_PARAM_ALL_REG(table, emc_trefbw,   REFBW);
    }

    if (TIMING_PRESET_SEVEN) {
        WRITE_PARAM_ALL_REG(table, emc_r2w,      R2W);
        WRITE_PARAM_ALL_REG(table, emc_w2r,      W2R);
        WRITE_PARAM_ALL_REG(table, emc_w2p,      WTP);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden,  WTPDEN);
        
        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = CEIL(WTP / MC_ARB_DIV);
        table->burst_mc_regs.mc_emem_arb_timing_r2w     = CEIL(R2W / MC_ARB_DIV) - 1 + MC_ARB_SFA;
        table->burst_mc_regs.mc_emem_arb_timing_w2r     = CEIL(W2R / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    }

    u32 DA_TURNS = 0;
    DA_TURNS |= u8(table->burst_mc_regs.mc_emem_arb_timing_r2w / 2) << 16; //R2W TURN
    DA_TURNS |= u8(table->burst_mc_regs.mc_emem_arb_timing_w2r / 2) << 24; //W2R TURN
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_da_turns,       DA_TURNS);
    u32 DA_COVERS = 0;
    u8 R_COVER = (table->burst_mc_regs.mc_emem_arb_timing_rap2pre + table->burst_mc_regs.mc_emem_arb_timing_rp + table->burst_mc_regs.mc_emem_arb_timing_rcd) / 2;
    u8 W_COVER = (table->burst_mc_regs.mc_emem_arb_timing_wap2pre + table->burst_mc_regs.mc_emem_arb_timing_rp + table->burst_mc_regs.mc_emem_arb_timing_rcd) / 2;
    DA_COVERS |= (u8)(table->burst_mc_regs.mc_emem_arb_timing_rc / 2); //RC COVER
    DA_COVERS |= (R_COVER << 8); //RCD_R COVER
    DA_COVERS |= (W_COVER << 16); //RCD_W COVER
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_da_covers,       DA_COVERS);
}

Result MemFreqMtcTable(u32* ptr) {
    u32 khz_list[] = { 1600000, 1331200, 1065600, 800000, 665600, 408000, 204000, 102000, 68000, 40800 };
    u32 khz_list_size = sizeof(khz_list) / sizeof(u32);

    // Generate list for mtc table pointers
    EristaMtcTable* table_list[khz_list_size];
    for (u32 i = 0; i < khz_list_size; i++) {
        u8* table = reinterpret_cast<u8 *>(ptr) - offsetof(EristaMtcTable, rate_khz) - i * sizeof(EristaMtcTable);
        table_list[i] = reinterpret_cast<EristaMtcTable *>(table);
        R_UNLESS(table_list[i]->rate_khz == khz_list[i],    ldr::ResultInvalidMtcTable());
        R_UNLESS(table_list[i]->rev == MTC_TABLE_REV,       ldr::ResultInvalidMtcTable());
    }

    if (C.eristaEmcMaxClock <= EmcClkOSLimit)
        R_SKIP();

    // Make room for new mtc table, discarding useless 40.8 MHz table
    // 40800 overwritten by 68000, ..., 1331200 overwritten by 1600000, leaving table_list[0] not overwritten
    for (u32 i = khz_list_size - 1; i > 0; i--)
        std::memcpy(static_cast<void *>(table_list[i]), static_cast<void *>(table_list[i - 1]), sizeof(EristaMtcTable));

    MemMtcTableAutoAdjust(table_list[0]);
    PATCH_OFFSET(ptr, C.eristaEmcMaxClock);

    // Handle customize table replacement
    //if (C.mtcConf == CUSTOMIZED_ALL) {
    //    MemMtcCustomizeTable(table_list[0], const_cast<EristaMtcTable *>(C.eristaMtcTable));
    //}

    R_SUCCEED();
}

Result MemFreqMax(u32* ptr) {
    if (C.eristaEmcMaxClock <= EmcClkOSLimit)
        R_SKIP();

    PATCH_OFFSET(ptr, C.eristaEmcMaxClock);

    R_SUCCEED();
}

void Patch(uintptr_t mapped_nso, size_t nso_size) {
    u32 CpuCvbDefaultMaxFreq = static_cast<u32>(GetDvfsTableLastEntry(CpuCvbTableDefault)->freq);
    u32 GpuCvbDefaultMaxFreq = static_cast<u32>(GetDvfsTableLastEntry(GpuCvbTableDefault)->freq);

    PatcherEntry<u32> patches[] = {
        { "CPU Freq Table", CpuFreqCvbTable<false>, 1, nullptr, CpuCvbDefaultMaxFreq },
        { "CPU Volt Limit", &CpuVoltRange,          0, &CpuMaxVoltPatternFn },
        { "GPU Freq Table", GpuFreqCvbTable<false>, 1, nullptr, GpuCvbDefaultMaxFreq },
        { "GPU Freq Asm",   &GpuFreqMaxAsm,         2, &GpuMaxClockPatternFn },
        { "GPU Freq PLL",   &GpuFreqPllLimit,       1, nullptr, GpuClkPllLimit },
        { "MEM Freq Mtc",   &MemFreqMtcTable,       0, nullptr, EmcClkOSLimit },
        { "MEM Freq Max",   &MemFreqMax,            0, nullptr, EmcClkOSLimit },
        { "MEM Freq PLLM",  &MemFreqPllmLimit,      2, nullptr, EmcClkPllmLimit },
        { "MEM Volt",       &MemVoltHandler,        2, nullptr, MemVoltHOS },
    };

    for (uintptr_t ptr = mapped_nso;
         ptr <= mapped_nso + nso_size - sizeof(EristaMtcTable);
         ptr += sizeof(u32))
    {
        u32* ptr32 = reinterpret_cast<u32 *>(ptr);
        for (auto& entry : patches) {
            if (R_SUCCEEDED(entry.SearchAndApply(ptr32)))
                break;
        }
    }

    for (auto& entry : patches) {
        LOGGING("%s Count: %zu", entry.description, entry.patched_count);
        if (R_FAILED(entry.CheckResult()))
            CRASH(entry.description);
    }
}

}