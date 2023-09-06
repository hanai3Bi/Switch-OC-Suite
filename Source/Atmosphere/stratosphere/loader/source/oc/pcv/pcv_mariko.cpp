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

#include "pcv.hpp"
#include "../mtc_timing_value.hpp"

namespace ams::ldr::oc::pcv::mariko {

Result CpuFreqVdd(u32* ptr) {
    dvfs_rail* entry = reinterpret_cast<dvfs_rail *>(reinterpret_cast<u8 *>(ptr) - offsetof(dvfs_rail, freq));

    R_UNLESS(entry->id == 1,            ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->min_mv == 250'000,  ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->step_mv == 5000,    ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->max_mv == 1525'000, ldr::ResultInvalidCpuFreqVddEntry());

    if (C.marikoCpuUV) {
        PATCH_OFFSET(ptr, GetDvfsTableLastEntry(C.marikoCpuDvfsTableSLT)->freq);
    } else {
        PATCH_OFFSET(ptr, GetDvfsTableLastEntry(C.marikoCpuDvfsTable)->freq);
    }
    
    R_SUCCEED();
}

Result CpuVoltRange(u32* ptr) {
    u32 min_volt_got = *(ptr - 1);
    for (const auto& mv : CpuMinVolts) {
        if (min_volt_got != mv)
            continue;

        if (!C.marikoCpuMaxVolt)
            R_SKIP();

        PATCH_OFFSET(ptr, C.marikoCpuMaxVolt);
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

    u32 max_clock;
    switch(C.marikoGpuUV) {
        case 0: 
            max_clock = GetDvfsTableLastEntry(C.marikoGpuDvfsTable)->freq;
            break;
        case 1: 
            max_clock = GetDvfsTableLastEntry(C.marikoGpuDvfsTableSLT)->freq;
            break;
        case 2: 
            max_clock = GetDvfsTableLastEntry(C.marikoGpuDvfsTableHiOPT)->freq;
            break;
        default: 
            max_clock = GetDvfsTableLastEntry(C.marikoGpuDvfsTable)->freq;
            break;
    }
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

void MemMtcTableAutoAdjust(MarikoMtcTable* table, const MarikoMtcTable* ref) {
    /* Official Tegra X1 TRM, sign up for nvidia developer program (free) to download:
     *     https://developer.nvidia.com/embedded/dlc/tegra-x1-technical-reference-manual
     *     Section 18.11: MC Registers
     *
     * Retail Mariko: 200FBGA 16Gb DDP LPDDR4X SDRAM x 2
     * x16/Ch, 1Ch/die, Double-die, 2Ch, 1CS(rank), 8Gb density per die
     * 64Mb x 16DQ x 8banks x 2channels = 2048MB (x32DQ) per package
     *
     * Devkit Mariko: 200FBGA 32Gb DDP LPDDR4X SDRAM x 2
     * x16/Ch, 1Ch/die, Quad-die,   2Ch, 2CS(rank), 8Gb density per die
     * X1+ EMC can R/W to both ranks at the same time, resulting in doubled DQ
     * 64Mb x 32DQ x 8banks x 2channels = 4096MB (x64DQ) per package
     *
     * If you have access to LPDDR4(X) specs or datasheets (from manufacturers or Google),
     * you'd better calculate timings yourself rather than relying on following algorithm.
     */

    if (C.mtcConf != AUTO_ADJ_ALL)
    	return;

    // scale with linear interpolation
    #define ADJUST_PROP(TARGET, REF)                                                                        \
        (u32)(CEIL((REF + ((C.marikoEmcMaxClock-EmcClkOSAlt)*(TARGET-REF))/(EmcClkOSLimit-EmcClkOSAlt))))

    #define ADJUST_PARAM(TARGET, REF)     \
        TARGET = ADJUST_PROP(TARGET, REF);

    #define ADJUST_PARAM_TABLE(TABLE, PARAM, REF)    ADJUST_PARAM(TABLE->PARAM, REF->PARAM)

    // Burst Register
    #define ADJUST_PARAM_ALL_REG(TABLE, PARAM, REF)                 \
        ADJUST_PARAM_TABLE(TABLE, burst_regs.PARAM, REF)            \
        ADJUST_PARAM_TABLE(TABLE, shadow_regs_ca_train.PARAM, REF)  \
        ADJUST_PARAM_TABLE(TABLE, shadow_regs_rdwr_train.PARAM, REF)

    #define WRITE_PARAM_BURST_REG(TABLE, PARAM, VALUE)          TABLE->burst_regs.PARAM = VALUE; 
    #define WRITE_PARAM_CA_TRAIN_REG(TABLE, PARAM, VALUE)       TABLE->shadow_regs_ca_train.PARAM = VALUE;
    #define WRITE_PARAM_RDWR_TRAIN_REG(TABLE, PARAM, VALUE)     TABLE->shadow_regs_rdwr_train.PARAM = VALUE;    

    #define WRITE_PARAM_ALL_REG(TABLE, PARAM, VALUE)    \
        WRITE_PARAM_BURST_REG(TABLE, PARAM, VALUE)      \
        WRITE_PARAM_CA_TRAIN_REG(TABLE, PARAM, VALUE)   \
        WRITE_PARAM_RDWR_TRAIN_REG(TABLE, PARAM, VALUE)

    #define GET_CYCLE_CEIL(PARAM) u32(CEIL(double(PARAM) / tCK_avg))
   
    WRITE_PARAM_ALL_REG(table, emc_rc,                  GET_CYCLE_CEIL(tRC));
    WRITE_PARAM_ALL_REG(table, emc_rfc,                 GET_CYCLE_CEIL(tRFCab));
    WRITE_PARAM_ALL_REG(table, emc_rfcpb,               GET_CYCLE_CEIL(tRFCpb));
    WRITE_PARAM_ALL_REG(table, emc_ras,                 GET_CYCLE_CEIL(tRAS));
    WRITE_PARAM_ALL_REG(table, emc_rp,                  GET_CYCLE_CEIL(tRPpb));
    WRITE_PARAM_ALL_REG(table, emc_r2w,                 R2W);
    WRITE_PARAM_ALL_REG(table, emc_w2r,                 W2R);
    WRITE_PARAM_ALL_REG(table, emc_r2p,                 GET_CYCLE_CEIL(tRTP));
    WRITE_PARAM_ALL_REG(table, emc_w2p,                 WTP);
    WRITE_PARAM_ALL_REG(table, emc_trtm,                RTM);
    WRITE_PARAM_ALL_REG(table, emc_twtm,                WTM);
    WRITE_PARAM_ALL_REG(table, emc_tratm,               RATM);
    WRITE_PARAM_ALL_REG(table, emc_twatm,               WATM);
    //WRITE_PARAM_ALL_REG(table, emc_tr2ref,              GET_CYCLE_CEIL(tR2REF));
    WRITE_PARAM_ALL_REG(table, emc_rd_rcd,              GET_CYCLE_CEIL(tRCD));
    WRITE_PARAM_ALL_REG(table, emc_wr_rcd,              GET_CYCLE_CEIL(tRCD));
    WRITE_PARAM_ALL_REG(table, emc_rrd,                 GET_CYCLE_CEIL(tRRD));
    WRITE_PARAM_ALL_REG(table, emc_rext,                26);
    WRITE_PARAM_ALL_REG(table, emc_refresh,             REFRESH);
    WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
    WRITE_PARAM_ALL_REG(table, emc_pdex2wr,             GET_CYCLE_CEIL(tXP));
    WRITE_PARAM_ALL_REG(table, emc_pdex2rd,             GET_CYCLE_CEIL(tXP));
    WRITE_PARAM_ALL_REG(table, emc_pchg2pden,           GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_act2pden,            GET_CYCLE_CEIL(tMRWCKEL));
    WRITE_PARAM_ALL_REG(table, emc_ar2pden,             GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_rw2pden,             WTPDEN);
    WRITE_PARAM_ALL_REG(table, emc_cke2pden,            GET_CYCLE_CEIL(tCKELCS));
    //WRITE_PARAM_ALL_REG(table, emc_pdex2cke,            GET_CYCLE_CEIL(tCSCKEH));
    WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,            GET_CYCLE_CEIL(tPDEX2MRR));
    WRITE_PARAM_ALL_REG(table, emc_txsr,                MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
    WRITE_PARAM_ALL_REG(table, emc_txsrdll,             MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
    WRITE_PARAM_ALL_REG(table, emc_tcke,                GET_CYCLE_CEIL(tCKE) + 1);
    WRITE_PARAM_ALL_REG(table, emc_tckesr,              GET_CYCLE_CEIL(tSR));
    WRITE_PARAM_ALL_REG(table, emc_tpd,                 GET_CYCLE_CEIL(tCKE));
    WRITE_PARAM_ALL_REG(table, emc_tfaw,                GET_CYCLE_CEIL(tFAW));
    WRITE_PARAM_ALL_REG(table, emc_trpab,               GET_CYCLE_CEIL(tRPab));
    //WRITE_PARAM_ALL_REG(table, emc_tclkstable,          GET_CYCLE_CEIL(tCKCKEH));
    WRITE_PARAM_ALL_REG(table, emc_tclkstop,            GET_CYCLE_CEIL(tCKE) + 8);
    WRITE_PARAM_ALL_REG(table, emc_trefbw,              REFBW);

    ADJUST_PARAM_ALL_REG(table, emc_dyn_self_ref_control, ref);


    #define CLEAR_BIT(BITS, HIGH, LOW)  \
        BITS = BITS & ~( ((1u << HIGH) << 1u) - (1u << LOW) );
    
    #define ADJUST(TARGET)              (u32)CEIL(TARGET * (C.marikoEmcMaxClock / EmcClkOSLimit))
    #define ADJUST_INVERSE(TARGET)      (u32)(TARGET * (EmcClkOSLimit / 1000) / (C.marikoEmcMaxClock / 1000))
    
    // Burst MC Regs
    #define WRITE_PARAM_BURST_MC_REG(TABLE, PARAM, VALUE)   TABLE->burst_mc_regs.PARAM = VALUE;

    constexpr u32 MC_ARB_DIV = 4;
    constexpr u32 MC_ARB_SFA = 2;

    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_cfg,            C.marikoEmcMaxClock / (33.3 * 1000) / MC_ARB_DIV); //CYCLES_PER_UPDATE: The number of mcclk cycles per deadline timer update
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rcd,     CEIL(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV) - 2)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rp,      CEIL(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV) - 1 + MC_ARB_SFA)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rc,      CEIL(GET_CYCLE_CEIL(tRC) / MC_ARB_DIV) - 1)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_ras,     CEIL(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV) - 2)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_faw,     CEIL(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rrd,     CEIL(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rap2pre, CEIL(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV))
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_wap2pre, CEIL((WTP) / MC_ARB_DIV))
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_r2r,     CEIL(table->burst_regs.emc_rext / MC_ARB_DIV) - 1 + MC_ARB_SFA)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_r2w,     CEIL((R2W) / MC_ARB_DIV) - 1 + MC_ARB_SFA)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_w2r,     CEIL((W2R) / MC_ARB_DIV) - 1 + MC_ARB_SFA)
    WRITE_PARAM_BURST_MC_REG(table, mc_emem_arb_timing_rfcpb,   CEIL(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV))

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

    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 7, 0);
    table->burst_mc_regs.mc_emem_arb_misc0 |= u8(table->burst_mc_regs.mc_emem_arb_timing_rc + 1); //BC2AA_HOLDOFF
    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 14, 8);
    table->burst_mc_regs.mc_emem_arb_misc0 |= u8((ADJUST(0x24) << 8)); //PRIORITY_INVERSION_THRESHOLD
    CLEAR_BIT(table->burst_mc_regs.mc_emem_arb_misc0, 20, 16);
    table->burst_mc_regs.mc_emem_arb_misc0 |= u8((ADJUST(12) << 16)); //PRIORITY_INVERSION_ISO_THRESHOLD

    // updown registers
    #define ADJUST_PARAM_LA_SCALE_REG(TABLE, PARAM) \
        TABLE->la_scale_regs.PARAM = ADJUST(TABLE->la_scale_regs.PARAM)

    #define ADJUST_PARAM_LA_SCALE_REG_HI(TABLE, PARAM, VALUE) \
        CLEAR_BIT(TABLE->la_scale_regs.PARAM, 23, 16)   \
        TABLE->la_scale_regs.PARAM |= VALUE << 16

    #define ADJUST_PARAM_LA_SCALE_REG_LO(TABLE, PARAM, VALUE) \
        CLEAR_BIT(TABLE->la_scale_regs.PARAM, 7, 0)   \
        TABLE->la_scale_regs.PARAM |= VALUE

    u8 LA = ADJUST_INVERSE(128); //0x80
    ADJUST_PARAM_LA_SCALE_REG(table, mc_mll_mpcorer_ptsa_rate); //208
    ADJUST_PARAM_LA_SCALE_REG(table, mc_ptsa_grant_decrement); //4611
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_xusb_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_xusb_1, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_tsec_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmca_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmcaa_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmc_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmcab_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmc_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_sdmmcab_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_ppcs_1, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_mpcore_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_avpc_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_gpu_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_gpu2_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_nvenc_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_nvdec_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_vic_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_HI(table, mc_latency_allowance_isp2_1, LA);

    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_hc_0, ADJUST_INVERSE(0x16));
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_hc_1, LA);
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_gpu_0, ADJUST_INVERSE(0x19));
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_gpu2_0, ADJUST_INVERSE(0x19));
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_vic_0, ADJUST_INVERSE(0x1d));
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_vi2_0, LA);
    ADJUST_PARAM_LA_SCALE_REG_LO(table, mc_latency_allowance_isp2_1, LA);

    //Spread Spectrum Control
    table->pllm_ss_ctrl1 = 0x0b55fe01;
    table->pllm_ss_ctrl2 = 0x10170b55;
    table->pllmb_ss_ctrl1 = 0x0b55fe01;
    table->pllmb_ss_ctrl2 = 0x10170b55;

    table->dram_timings.t_rp = tRPpb;
    table->dram_timings.t_rfc = tRFCab;
    //table->dram_timings.rl = 32;

    table->emc_cfg_2 = 0x0011083d;
}

void MemMtcTableCustomAdjust(MarikoMtcTable* table) {
    if (C.mtcConf != CUSTOM_ADJ_ALL)
    	return;
       
    constexpr u32 MC_ARB_DIV = 4;
    constexpr u32 MC_ARB_SFA = 2;

    if (TIMING_PRESET_ONE) {
        WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC));
        WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS));
        WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb));
        WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab));
        WRITE_PARAM_ALL_REG(table, emc_rd_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_wr_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR));

        table->burst_mc_regs.mc_emem_arb_timing_rcd     = CEIL(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV) - 2;
        table->burst_mc_regs.mc_emem_arb_timing_rc      = CEIL(GET_CYCLE_CEIL(tRC) / MC_ARB_DIV) - 1;
        table->burst_mc_regs.mc_emem_arb_timing_rp      = CEIL(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV) - 1 + MC_ARB_SFA;
        table->burst_mc_regs.mc_emem_arb_timing_ras     = CEIL(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV) - 2;
        
    }
    
    if (TIMING_PRESET_TWO) {
        WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW));
        WRITE_PARAM_ALL_REG(table, emc_rrd,     GET_CYCLE_CEIL(tRRD));

        table->burst_mc_regs.mc_emem_arb_timing_faw     = CEIL(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1;
        table->burst_mc_regs.mc_emem_arb_timing_rrd     = CEIL(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1;
    }

    if (TIMING_PRESET_THREE) {
        WRITE_PARAM_ALL_REG(table, emc_r2p,     GET_CYCLE_CEIL(tRTP));
        WRITE_PARAM_ALL_REG(table, emc_w2p,     WTP);
        WRITE_PARAM_ALL_REG(table, emc_tratm,   RATM);
        WRITE_PARAM_ALL_REG(table, emc_twatm,   WATM);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden, WTPDEN);

        table->burst_mc_regs.mc_emem_arb_timing_rap2pre = CEIL(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV);
        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = CEIL(WTP / MC_ARB_DIV);
    }

    if (TIMING_PRESET_FOUR) {
        WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab));
        WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb));
        WRITE_PARAM_ALL_REG(table, emc_txsr,    MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
        WRITE_PARAM_ALL_REG(table, emc_txsrdll, MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));

        table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = CEIL(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV);
    }

    if (TIMING_PRESET_FIVE) {
        WRITE_PARAM_ALL_REG(table, emc_w2r, W2R);

        table->burst_mc_regs.mc_emem_arb_timing_w2r     = CEIL(W2R / MC_ARB_DIV) - 1 + MC_ARB_SFA;
    }

    if (TIMING_PRESET_SIX) {
        WRITE_PARAM_ALL_REG(table, emc_refresh, REFRESH);
        WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
        WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFBW);
    }

    if (TIMING_PRESET_SEVEN) {
        WRITE_PARAM_ALL_REG(table, emc_r2w,     R2W);
        WRITE_PARAM_ALL_REG(table, emc_w2r,     W2R);
        WRITE_PARAM_ALL_REG(table, emc_w2p,     WTP);
        WRITE_PARAM_ALL_REG(table, emc_trtm,    RTM);
        WRITE_PARAM_ALL_REG(table, emc_twtm,    WTM);
        WRITE_PARAM_ALL_REG(table, emc_tratm,   RATM);
        WRITE_PARAM_ALL_REG(table, emc_twatm,   WATM);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden, WTPDEN);

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

void MemMtcPllmbDivisor(MarikoMtcTable* table) {
    // Calculate DIVM and DIVN (clock divisors)
    // Common PLL oscillator is 38.4 MHz
    // PLLMB_OUT = 38.4 MHz / PLLLMB_DIVM * PLLMB_DIVN
    typedef struct {
        u8 numerator   : 4;
        u8 denominator : 4;
    } pllmb_div;

    constexpr pllmb_div div[] = {
        {3, 4}, {2, 3}, {1, 2}, {1, 3}, {1, 4}, {0, 2}
    };

    constexpr u32 pll_osc_in = 38'400;
    u32 divm {}, divn {};
    const u32 remainder = C.marikoEmcMaxClock % pll_osc_in;
    for (const auto &index : div) {
        // Round down
        if (remainder >= pll_osc_in * index.numerator / index.denominator) {
            divm = index.denominator;
            divn = C.marikoEmcMaxClock / pll_osc_in * divm + index.numerator;
            break;
        }
    }

    table->pllmb_divm = divm;
    table->pllmb_divn = divn;
}

Result MemFreqMtcTable(u32* ptr) {
    u32 khz_list[] = { 1600000, 1331200, 204000 };
    u32 khz_list_size = sizeof(khz_list) / sizeof(u32);

    // Generate list for mtc table pointers
    MarikoMtcTable* table_list[khz_list_size];
    for (u32 i = 0; i < khz_list_size; i++) {
        u8* table = reinterpret_cast<u8 *>(ptr) - offsetof(MarikoMtcTable, rate_khz) - i * sizeof(MarikoMtcTable);
        table_list[i] = reinterpret_cast<MarikoMtcTable *>(table);
        R_UNLESS(table_list[i]->rate_khz == khz_list[i],    ldr::ResultInvalidMtcTable());
        R_UNLESS(table_list[i]->rev == MTC_TABLE_REV,       ldr::ResultInvalidMtcTable());
    }

    if (C.marikoEmcMaxClock <= EmcClkOSLimit)
        R_SKIP();

    MarikoMtcTable *table_alt = table_list[1], *table_max = table_list[0];
    MarikoMtcTable *tmp = new MarikoMtcTable;

    // Copy unmodified 1600000 table to tmp
    std::memcpy(reinterpret_cast<void *>(tmp), reinterpret_cast<void *>(table_max), sizeof(MarikoMtcTable));
    // Adjust max freq mtc timing parameters with reference to 1331200 table
    MemMtcTableAutoAdjust(table_max, table_alt);
    MemMtcTableCustomAdjust(table_max);
    MemMtcPllmbDivisor(table_max);
    // Overwrite 13312000 table with unmodified 1600000 table copied back
    std::memcpy(reinterpret_cast<void *>(table_alt), reinterpret_cast<void *>(tmp), sizeof(MarikoMtcTable));

    delete tmp;

    PATCH_OFFSET(ptr, C.marikoEmcMaxClock);

    // Handle customize table replacement
    //if (C.mtcConf == CUSTOMIZED_ALL) {
    //    MemMtcCustomizeTable(table_list[0], reinterpret_cast<MarikoMtcTable *>(reinterpret_cast<u8 *>(C.marikoMtcTable)));
    // }

    R_SUCCEED();
}

Result MemFreqDvbTable(u32* ptr) {
    emc_dvb_dvfs_table_t* default_end  = reinterpret_cast<emc_dvb_dvfs_table_t *>(ptr);
    emc_dvb_dvfs_table_t* new_start = default_end + 1;

    // Validate existing table
    void* mem_dvb_table_head = reinterpret_cast<u8 *>(new_start) - sizeof(EmcDvbTableDefault);
    bool validated = std::memcmp(mem_dvb_table_head, EmcDvbTableDefault, sizeof(EmcDvbTableDefault)) == 0;
    R_UNLESS(validated, ldr::ResultInvalidDvbTable());

    if (C.marikoEmcMaxClock <= EmcClkOSLimit)
        R_SKIP();

    u32 voltAdd = 25*C.marikoEmcDvbShift;
    if (C.marikoEmcMaxClock < 1862400) {
        std::memcpy(new_start, default_end, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock < 2131200){
        emc_dvb_dvfs_table_t oc_table = { 1862400, { 700, 675, 650, } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock < 2400000){
        emc_dvb_dvfs_table_t oc_table = { 2131200, { 725, 700, 675, } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock < 2665600){
        emc_dvb_dvfs_table_t oc_table = { 2400000, { s32(750+voltAdd), s32(725+voltAdd), s32(700+voltAdd), } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock < 2931200){
        emc_dvb_dvfs_table_t oc_table = { 2665600, { s32(775+voltAdd), s32(750+voltAdd), s32(725+voltAdd), } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock < 3200000){
        emc_dvb_dvfs_table_t oc_table = { 2931200, { s32(800+voltAdd), s32(775+voltAdd), s32(750+voltAdd), } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else {
        emc_dvb_dvfs_table_t oc_table = { 3200000, { s32(800+voltAdd), s32(800+voltAdd), s32(775+voltAdd), } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    }
    new_start->freq = C.marikoEmcMaxClock;
    /* Max dvfs entry is 32, but HOS doesn't seem to boot if exact freq doesn't exist in dvb table,
       reason why it's like this 
    */ 

    R_SUCCEED();
}

Result MemFreqMax(u32* ptr) {
    if (C.marikoEmcMaxClock <= EmcClkOSLimit)
        R_SKIP();

    PATCH_OFFSET(ptr, C.marikoEmcMaxClock);
    R_SUCCEED();
}

Result EmcVddqVolt(u32* ptr) {
    regulator* entry = reinterpret_cast<regulator *>(reinterpret_cast<u8 *>(ptr) - offsetof(regulator, type_2_3.default_uv));

    constexpr u32 uv_step = 5'000;
    constexpr u32 uv_min  = 250'000;

    auto validator = [entry]() {
        R_UNLESS(entry->id == 2,                        ldr::ResultInvalidRegulatorEntry());
        R_UNLESS(entry->type == 3,                      ldr::ResultInvalidRegulatorEntry());
        R_UNLESS(entry->type_2_3.step_uv == uv_step,    ldr::ResultInvalidRegulatorEntry());
        R_UNLESS(entry->type_2_3.min_uv == uv_min,      ldr::ResultInvalidRegulatorEntry());
        R_SUCCEED();
    };

    R_TRY(validator());

    u32 emc_uv = C.marikoEmcVddqVolt;
    if (!emc_uv)
        R_SKIP();

    if (emc_uv % uv_step)
        emc_uv = emc_uv / uv_step * uv_step; // rounding

    PATCH_OFFSET(ptr, emc_uv);

    R_SUCCEED();
}

void Patch(uintptr_t mapped_nso, size_t nso_size) {
    u32 CpuCvbDefaultMaxFreq = static_cast<u32>(GetDvfsTableLastEntry(CpuCvbTableDefault)->freq);
    u32 GpuCvbDefaultMaxFreq = static_cast<u32>(GetDvfsTableLastEntry(GpuCvbTableDefault)->freq);

    PatcherEntry<u32> patches[] = {
        { "CPU Freq Vdd",   &CpuFreqVdd,            1, nullptr, CpuClkOSLimit },
        { "CPU Freq Table", CpuFreqCvbTable<true>,  1, nullptr, CpuCvbDefaultMaxFreq },
        { "CPU Volt Limit", &CpuVoltRange,         13, nullptr, CpuVoltOfficial },
        { "GPU Freq Table", GpuFreqCvbTable<true>,  1, nullptr, GpuCvbDefaultMaxFreq },
        { "GPU Freq Asm",   &GpuFreqMaxAsm,         2, &GpuMaxClockPatternFn },
        //{ "GPU Freq PLL",   &GpuFreqPllLimit,       1, nullptr, GpuClkPllLimit },
        { "MEM Freq Mtc",   &MemFreqMtcTable,       0, nullptr, EmcClkOSLimit },
        { "MEM Freq Dvb",   &MemFreqDvbTable,       1, nullptr, EmcClkOSLimit },
        { "MEM Freq Max",   &MemFreqMax,            0, nullptr, EmcClkOSLimit },
        { "MEM Freq PLLM",  &MemFreqPllmLimit,      2, nullptr, EmcClkPllmLimit },
        { "MEM Vddq",       &EmcVddqVolt,           2, nullptr, EmcVddqDefault },
        { "MEM Vdd2",       &MemVoltHandler,        2, nullptr, MemVdd2Default }
    };

    for (uintptr_t ptr = mapped_nso;
         ptr <= mapped_nso + nso_size - sizeof(MarikoMtcTable);
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