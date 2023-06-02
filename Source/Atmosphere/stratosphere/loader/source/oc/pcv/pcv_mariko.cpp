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

#define MAX(A, B)   std::max(A, B)
#define MIN(A, B)   std::min(A, B)
#define CEIL(A)     std::ceil(A)
#define FLOOR(A)    std::floor(A)

namespace ams::ldr::oc::pcv::mariko {

Result CpuFreqVdd(u32* ptr) {
    dvfs_rail* entry = reinterpret_cast<dvfs_rail *>(reinterpret_cast<u8 *>(ptr) - offsetof(dvfs_rail, freq));

    R_UNLESS(entry->id == 1,            ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->min_mv == 250'000,  ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->step_mv == 5000,    ldr::ResultInvalidCpuFreqVddEntry());
    R_UNLESS(entry->max_mv == 1525'000, ldr::ResultInvalidCpuFreqVddEntry());

    PATCH_OFFSET(ptr, GetDvfsTableLastEntry(C.marikoCpuDvfsTable)->freq);

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

    u32 max_clock = GetDvfsTableLastEntry(C.marikoGpuDvfsTable)->freq;
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

const std::array<u32, 6> tRCD_values = {18, 17, 16, 15, 14, 13};
const std::array<u32, 6> tRP_values = {18, 17, 16, 15, 14, 13};
const std::array<u32, 6> tRAS_values = {42, 39, 36, 34, 32, 30};

const std::array<double, 5> tRRD_values = {10, 7.5, 6, 4, 2};
const std::array<double, 5> tFAW_values = {40, 30, 24, 16, 8};

const std::array<u32, 5> tWR_values = {18, 15, 15, 12, 8};
const std::array<double, 5> tRTP_values = {7.5, 7.5, 6, 6, 4};

const std::array<u32, 5> tRFC_values = {140, 120, 100, 80, 60};

const std::array<u32, 5> tWTR_values = {10, 9, 8, 7, 6};

const std::array<u32, 4> tREFpb_values = {488, 976, 1952, 3256};

const std::array<u32, 6> tWL_values = {14, 12, 10, 8, 6, 4};

const u32 TIMING_PRESET_ONE = C.ramTimingPresetOne;
const u32 TIMING_PRESET_TWO = C.ramTimingPresetTwo;
const u32 TIMING_PRESET_THREE = C.ramTimingPresetThree;
const u32 TIMING_PRESET_FOUR = C.ramTimingPresetFour;
const u32 TIMING_PRESET_FIVE = C.ramTimingPresetFive;
const u32 TIMING_PRESET_SIX = C.ramTimingPresetSix;
const u32 TIMING_PRESET_SEVEN = C.ramTimingPresetSeven;

// tCK_avg (average clock period) in ns
const double tCK_avg = 1000'000. / C.marikoEmcMaxClock;

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

    if (C.mtcConf != AUTO_ADJ_SAFE_MARIKO)
    	return;

    #define ADJUST_PROP(TARGET, REF)                                                                        \
        (u32)(std::ceil((REF + ((C.marikoEmcMaxClock-EmcClkOSAlt)*(TARGET-REF))/(EmcClkOSLimit-EmcClkOSAlt))))

    #define ADJUST_PARAM(TARGET, REF)     \
        TARGET = ADJUST_PROP(TARGET, REF);

    #define ADJUST_PARAM_TABLE(TABLE, PARAM, REF)    ADJUST_PARAM(TABLE->PARAM, REF->PARAM)

    #define ADJUST_PARAM_ALL_REG(TABLE, PARAM, REF)                 \
        ADJUST_PARAM_TABLE(TABLE, burst_regs.PARAM, REF)            \
        ADJUST_PARAM_TABLE(TABLE, shadow_regs_ca_train.PARAM, REF)  \
        ADJUST_PARAM_TABLE(TABLE, shadow_regs_rdwr_train.PARAM, REF)

    #define WRITE_PARAM_ALL_REG(TABLE, PARAM, VALUE)\
    TABLE->burst_regs.PARAM = VALUE;            \
    TABLE->shadow_regs_ca_train.PARAM = VALUE;  \
    TABLE->shadow_regs_rdwr_train.PARAM = VALUE;

    //ADJUST_PARAM_ALL_REG(table, emc_r2w, ref); //0x140 0x4f0 0x880
    //ADJUST_PARAM_ALL_REG(table, emc_w2r, ref); //0x144 0x4f4 0x884
    //ADJUST_PARAM_ALL_REG(table, emc_r2p, ref); //0x148 0x4f8 0x888
    //ADJUST_PARAM_ALL_REG(table, emc_w2p, ref); //0x14c 0x4fc 0x88c
    ADJUST_PARAM_ALL_REG(table, emc_trtm, ref); //0x158 0x508 0x898
    ADJUST_PARAM_ALL_REG(table, emc_twtm, ref); //0x15c 0x50c 0x89c
    ADJUST_PARAM_ALL_REG(table, emc_tratm, ref); //0x160 0x510 0x8a0
    ADJUST_PARAM_ALL_REG(table, emc_twatm, ref); //0x164 0x514 0x8a4

    //ADJUST_PARAM_ALL_REG(table, emc_rw2pden, ref); //0x1fc 0x5ac 0x93c

    ADJUST_PARAM_ALL_REG(table, emc_tclkstop, ref); //0x22c 0x5dc 0x96c

    ADJUST_PARAM_ALL_REG(table, emc_pmacro_dll_cfg_2, ref); // EMC_DLL_CFG_2_0: level select for VDDA? //0x380 0x730 0xac0

    ADJUST_PARAM_TABLE(table, la_scale_regs.mc_mll_mpcorer_ptsa_rate, ref); //0xfa4
    ADJUST_PARAM_TABLE(table, la_scale_regs.mc_ptsa_grant_decrement, ref); //0xfac

    /* Timings that are available in or can be derived from LPDDR4X datasheet or TRM */
    
    // tRPpb (row precharge time per bank) in ns
    const u32 tRPpb = !TIMING_PRESET_ONE ? 18 : tRP_values[TIMING_PRESET_ONE-1];
    // tRPab (row precharge time all banks) in ns
    const u32 tRPab = !TIMING_PRESET_ONE ? 21 : tRPpb + 3;
    // tRAS (row active time) in ns
    const u32 tRAS = !TIMING_PRESET_ONE ? 42 : tRAS_values[TIMING_PRESET_ONE-1];
    // tRC (ACTIVATE-ACTIVATE command period same bank) in ns
    const u32 tRC = tRPpb + tRAS;
    // tRFCpb (refresh cycle time per bank) in ns for 8Gb density
    const u32 tRFCpb = !TIMING_PRESET_FOUR ? 140 : tRFC_values[TIMING_PRESET_FOUR-1];
    // tRFCab (refresh cycle time all banks) in ns for 8Gb density
    const u32 tRFCab = !TIMING_PRESET_FOUR ? 280 : 2*tRFCpb;
    // tRCD (RAS-CAS delay) in ns
    const u32 tRCD = !TIMING_PRESET_ONE ? 18 : tRCD_values[TIMING_PRESET_ONE-1];
    // tRRD (Active bank-A to Active bank-B) in ns
    const double tRRD = !TIMING_PRESET_TWO ? 10. : tRRD_values[TIMING_PRESET_TWO-1];
    // tREFpb (average refresh interval per bank) in ns for 8Gb density
    const u32 tREFpb = !TIMING_PRESET_SIX ? 488 : tREFpb_values[TIMING_PRESET_SIX-1];
    // tREFab (average refresh interval all 8 banks) in ns for 8Gb density
    // const u32 tREFab = tREFpb * 8;
    // #_of_rows per die for 8Gb density
    const u32 numOfRows = 65536;
    // {REFRESH, REFRESH_LO} = max[(tREF/#_of_rows) / (emc_clk_period) - 64, (tREF/#_of_rows) / (emc_clk_period) * 97%]
    // emc_clk_period = dram_clk / 2;
    // 1600 MHz: 5894, but N' set to 6176 (~4.8% margin)
    const u32 REFRESH = u32(std::ceil((double(tREFpb) * C.marikoEmcMaxClock / numOfRows * 1.048 / 2 - 64))) / 4 * 4;
    // tPDEX2WR, tPDEX2RD (timing delay from exiting powerdown mode to a write/read command) in ns
    //const u32 tPDEX2 = 10;
    // tACT2PDEN (timing delay from an activate, MRS or EMRS command to power-down entry) in ns
    // Valid clock and CS requirement after CKE input LOW after MRW command
    const u32 tMRWCKEL = 14;
    // Additional time after t XP hasexpired until the MRR commandmay be issued
    const double tMRRI = tRCD + 3 * tCK_avg;
    // Exit power-down to next valid command delay
    const double tXP = 7.5;
    // tPDEX2WR, tPDEX2RD (timing delay from exiting powerdown mode to a write/read command) in ns
    //const u32 tPDEX2 = 10;
    // tPDEX2MRR (timing delay from exiting powerdown mode to MRR command) in ns
    const double tPDEX2MRR = tXP + tMRRI;
    // [Guessed] tPDEX2MRR (timing delay from exiting powerdown mode to MRR command) in ns
    //const double tPDEX2MRR = 28.75;
    // [Guessed] tCKE2PDEN (timing delay from turning off CKE to power-down entry) in ns
    const double tCKE2PDEN = 8.5;
    // tXSR (SELF REFRESH exit to next valid command delay) in ns
    const double tXSR = tRFCab + 7.5;
    // tCKE (minimum CKE high pulse width) in ns
    const double tCKE = 7;
    // Delay from valid command to CKE input LOW in ns
    const double tCMDCKE = MAX(1.75, 3*tCK_avg);
    // Minimum self refresh time (entry to exit)
    const u32 tSR = 15;
    // [Guessed] tPD (minimum CKE low pulse width in power-down mode) in ns
    const double tPD = 7.5;
    // tFAW (Four-bank Activate Window) in ns
    const u32 tFAW = !TIMING_PRESET_TWO ? 40 : tFAW_values[TIMING_PRESET_TWO-1];

    // Internal READ-to-PRE-CHARGE command delay in ns
    const double tRTP = !TIMING_PRESET_THREE ? 7.5 : tRTP_values[TIMING_PRESET_THREE-1];
    const u32 WL = !TIMING_PRESET_SEVEN ? (C.marikoEmcMaxClock <= 2131200 ? 10 : 12) : tWL_values[TIMING_PRESET_SEVEN-1]; //?
    const u32 BL = 16;

    const u32 tWR = !TIMING_PRESET_THREE ? 18 : tWR_values[TIMING_PRESET_THREE-1];
    // write-to-precharge time for commands to the same bank in cycles
    const u32 WTP = WL + BL/2 + 1 + CEIL(tWR/tCK_avg);

    const double tDQSS_max = 1.25;
    const double tDQS2DQ_max = 0.8;
    // Write With Auto Precharge to to Power-Down Entry
    const u32 WTPDEN = WTP + 1 + CEIL(tDQSS_max/tCK_avg) + CEIL(tDQS2DQ_max/tCK_avg) + 6;

    // Valid Clock requirement before CKE Input HIGH in ns
    const double tCKCKEH = MAX(1.75, 3*tCK_avg);

    // Write-to-Read delay
    const u32 tWTR = !TIMING_PRESET_FIVE ? 10 : tWTR_values[TIMING_PRESET_FIVE-1];
    // Delay Time From WRITE-to-READ
    const u32 W2R = WL + BL/2 + 1 + CEIL(tWTR/tCK_avg);

    const u32 RL = !TIMING_PRESET_SEVEN ? (C.marikoEmcMaxClock <= 2131200 ? 20 : 24) : WL*2; //?
    const double tDQSCK_max = 3.5;
    // Write preamble (tCK)
    const double tWPRE = 1.8;
    // Read postamble (tck)
    const double tRPST = 0.4;
    const u32 R2W = CEIL (RL + CEIL(tDQSCK_max/tCK_avg) + BL/2 - WL + tWPRE + FLOOR(tRPST));

    #define GET_CYCLE_CEIL(PARAM) u32(std::ceil(double(PARAM) / tCK_avg))

    WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC)); //0x124
    WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab)); //0x128
    WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb)); //0x12c
    WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS)); //0x138
    WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb)); //0x13c
    WRITE_PARAM_ALL_REG(table, emc_w2r,     W2R);
    WRITE_PARAM_ALL_REG(table, emc_r2w,     R2W);
    WRITE_PARAM_ALL_REG(table, emc_r2p,     GET_CYCLE_CEIL(tRTP));
    WRITE_PARAM_ALL_REG(table, emc_w2p,     WTP);
    WRITE_PARAM_ALL_REG(table, emc_rd_rcd,  GET_CYCLE_CEIL(tRCD)); //0x170
    WRITE_PARAM_ALL_REG(table, emc_wr_rcd,  GET_CYCLE_CEIL(tRCD)); //0x174
    WRITE_PARAM_ALL_REG(table, emc_rrd,     GET_CYCLE_CEIL(tRRD)); //0x178
    WRITE_PARAM_ALL_REG(table, emc_refresh, REFRESH); //0x1dc
    WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4); //0x1e4
    WRITE_PARAM_ALL_REG(table, emc_pdex2wr, GET_CYCLE_CEIL(tXP)); //0x1e8
    WRITE_PARAM_ALL_REG(table, emc_pdex2rd, GET_CYCLE_CEIL(tXP)); //0x1ec
    WRITE_PARAM_ALL_REG(table, emc_pchg2pden, GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_act2pden,GET_CYCLE_CEIL(tMRWCKEL)); //0x1f4
    WRITE_PARAM_ALL_REG(table, emc_ar2pden, GET_CYCLE_CEIL(tCMDCKE));
    WRITE_PARAM_ALL_REG(table, emc_rw2pden, WTPDEN);
    WRITE_PARAM_ALL_REG(table, emc_txsr,    MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe)); //0x20c
    WRITE_PARAM_ALL_REG(table, emc_txsrdll, MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe)); //0x210
    WRITE_PARAM_ALL_REG(table, emc_tcke,    GET_CYCLE_CEIL(tCKE)); //0x214
    WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW)); //0x220
    WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab)); //0x224
    WRITE_PARAM_ALL_REG(table, emc_tclkstable, GET_CYCLE_CEIL(tCKCKEH));
    WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFRESH + 64); //0x234
    WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR)); //0x208
    WRITE_PARAM_ALL_REG(table, emc_cke2pden,GET_CYCLE_CEIL(tCKE2PDEN)); //0x200
    WRITE_PARAM_ALL_REG(table, emc_tckesr,  GET_CYCLE_CEIL(tSR)); //0x218
    WRITE_PARAM_ALL_REG(table, emc_tpd,     GET_CYCLE_CEIL(tPD)); //0x21c
    
    constexpr u32 MC_ARB_DIV = 4; // Guessed
    constexpr u32 SFA = 2; // Guessed
    table->burst_mc_regs.mc_emem_arb_timing_rcd     = std::ceil(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV) - 2; //0xf30
    table->burst_mc_regs.mc_emem_arb_timing_rp      = std::ceil(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV) - 1 + SFA; //0xf34
    table->burst_mc_regs.mc_emem_arb_timing_rc      = std::ceil(std::max(GET_CYCLE_CEIL(tRC), GET_CYCLE_CEIL(tRAS)+GET_CYCLE_CEIL(tRPpb)) / MC_ARB_DIV) - 1; //0xf38
    table->burst_mc_regs.mc_emem_arb_timing_ras     = std::ceil(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV) - 2; //0xf3c
    table->burst_mc_regs.mc_emem_arb_timing_faw     = std::ceil(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1; //0xf40
    table->burst_mc_regs.mc_emem_arb_timing_rrd     = std::ceil(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1; //0xf44
    table->burst_mc_regs.mc_emem_arb_timing_rap2pre = std::ceil(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV); //0xf48
    table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(WTP / MC_ARB_DIV); //0xf4c
    //table->burst_mc_regs.mc_emem_arb_timing_r2r     = std::ceil(table->burst_regs.emc_rext / MC_ARB_DIV) - 1 + SFA;
    //table->burst_mc_regs.mc_emem_arb_timing_w2w     = std::ceil(table->burst_regs.emc_wext / MC_ARB_DIV) - 1 + SFA;
    table->burst_mc_regs.mc_emem_arb_timing_r2w     = std::ceil(R2W / MC_ARB_DIV) - 1 + SFA; //0xf58
    table->burst_mc_regs.mc_emem_arb_timing_w2r     = std::ceil(W2R / MC_ARB_DIV) - 1 + SFA; //0xf60
    table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = std::ceil(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV); //0xf64
    //table->burst_mc_regs.mc_emem_arb_timing_ccdmw   = std::ceil(tCCDMW / MC_ARB_DIV) -1 + SFA;
}

void MemMtcTableCustomAdjust(MarikoMtcTable* table) {
    if (C.mtcConf != CUSTOM_ADJ_MARIKO)
    	return;
       
    const u32 i_one = TIMING_PRESET_ONE == 0 ? 0 : TIMING_PRESET_ONE -1;
    const u32 tRCD = tRCD_values[i_one];
    const u32 tRPpb = tRP_values[i_one];
    const u32 tRPab = tRPpb + 3;
    const u32 tRAS = tRAS_values[i_one];
    const u32 tRC = tRAS + tRPpb; 

    const double tMRRI = tRCD + 3 * tCK_avg;
    const double tXP = 7.5;
    const double tPDEX2MRR = tXP + tMRRI;
    
    const u32 i_two = TIMING_PRESET_TWO == 0 ? 0 : TIMING_PRESET_TWO - 1;
    const double tRRD = tRRD_values[i_two];
    const u32 tFAW = tFAW_values[i_two];

    const u32 i_three = TIMING_PRESET_THREE == 0 ? 0 : TIMING_PRESET_THREE - 1;
    const double tRTP = tRTP_values[i_three];
    const u32 tWR = tWR_values[i_three];
    const u32 WL = !TIMING_PRESET_SEVEN ? (C.marikoEmcMaxClock <= 2131200 ? 10 : 12) : tWL_values[TIMING_PRESET_SEVEN-1];
    const u32 BL = 16;
    const u32 WTP = WL + BL/2 + 1 + CEIL(tWR/tCK_avg);
    const double tDQSS_max = 1.25;
    const double tDQS2DQ_max = 0.8;
    const u32 WTPDEN = WTP + 1 + CEIL(tDQSS_max/tCK_avg) + CEIL(tDQS2DQ_max/tCK_avg) + 6;

    const u32 i_four = TIMING_PRESET_FOUR == 0 ? 0 : TIMING_PRESET_FOUR - 1;
    const u32 tRFCpb = tRFC_values[i_four];
    const u32 tRFCab = 2*tRFCpb;
    const double tXSR = tRFCab + 7.5;

    const u32 i_five = TIMING_PRESET_FIVE == 0 ? 0 : TIMING_PRESET_FIVE - 1;
    const u32 tWTR = tWTR_values[i_five];
    const u32 W2R = WL + BL/2 + 1 + CEIL(tWTR/tCK_avg);

    const u32 i_six = TIMING_PRESET_SIX == 0 ? 0 : TIMING_PRESET_SIX - 1;
    const u32 tREFpb = tREFpb_values[i_six];
    const u32 numOfRows = 65536;
    const u32 REFRESH = u32(std::ceil((double(tREFpb) * C.marikoEmcMaxClock / numOfRows * 1.048 / 2 - 64))) / 4 * 4;
  
    const u32 RL = !TIMING_PRESET_SEVEN ? (C.marikoEmcMaxClock <= 2131200 ? 20 : 24) : WL*2; //?
    const double tDQSCK_max = 3.5;
    // Write preamble (tCK)
    const double tWPRE = 1.8;
    // Read postamble (tck)
    const double tRPST = 0.4;
    const u32 R2W = CEIL (RL + CEIL(tDQSCK_max/tCK_avg) + BL/2 - WL + tWPRE + FLOOR(tRPST));
    
    constexpr u32 MC_ARB_DIV = 4;
    constexpr u32 SFA = 2;

    if (TIMING_PRESET_ONE) {
        WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC));
        WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS));
        WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb));
        WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab));
        WRITE_PARAM_ALL_REG(table, emc_rd_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_wr_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR));

        table->burst_mc_regs.mc_emem_arb_timing_rcd     = std::ceil(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV - 2);
        table->burst_mc_regs.mc_emem_arb_timing_rc      = std::ceil(GET_CYCLE_CEIL(tRC) / MC_ARB_DIV - 1);
        table->burst_mc_regs.mc_emem_arb_timing_rp      = std::ceil(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV - 1 + SFA);
        table->burst_mc_regs.mc_emem_arb_timing_ras     = std::ceil(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV - 2);
        
    }
    if (TIMING_PRESET_TWO) {
        WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW));
        WRITE_PARAM_ALL_REG(table, emc_rrd,     GET_CYCLE_CEIL(tRRD));

        table->burst_mc_regs.mc_emem_arb_timing_faw     = std::ceil(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1;
        table->burst_mc_regs.mc_emem_arb_timing_rrd     = std::ceil(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1;
    }

    if (TIMING_PRESET_THREE) {
        WRITE_PARAM_ALL_REG(table, emc_r2p,     GET_CYCLE_CEIL(tRTP));
        WRITE_PARAM_ALL_REG(table, emc_w2p,     WTP);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden, WTPDEN);

        table->burst_mc_regs.mc_emem_arb_timing_rap2pre = std::ceil(GET_CYCLE_CEIL(tRTP) / MC_ARB_DIV);
        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(WTP / MC_ARB_DIV);
    }

    if (TIMING_PRESET_FOUR) {
        WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab));
        WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb));
        WRITE_PARAM_ALL_REG(table, emc_txsr,    MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));
        WRITE_PARAM_ALL_REG(table, emc_txsrdll, MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe));

        table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = std::ceil(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV);
    }
    if (TIMING_PRESET_FIVE) {
        WRITE_PARAM_ALL_REG(table, emc_w2r, W2R);

        table->burst_mc_regs.mc_emem_arb_timing_w2r     = std::ceil(W2R / MC_ARB_DIV) - 1 + SFA;
    }

    if (TIMING_PRESET_SIX) {
        WRITE_PARAM_ALL_REG(table, emc_refresh, REFRESH);
        WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
        WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFRESH + 64);
    }

    if (TIMING_PRESET_SEVEN) {
        WRITE_PARAM_ALL_REG(table, emc_w2r,     W2R);
        WRITE_PARAM_ALL_REG(table, emc_w2p,     WTP);
        WRITE_PARAM_ALL_REG(table, emc_rw2pden, WTPDEN);
        WRITE_PARAM_ALL_REG(table, emc_r2w,     R2W);

        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(WTP / MC_ARB_DIV);
        table->burst_mc_regs.mc_emem_arb_timing_r2w     = std::ceil(R2W / MC_ARB_DIV) - 1 + SFA;
        table->burst_mc_regs.mc_emem_arb_timing_w2r     = std::ceil(W2R / MC_ARB_DIV) - 1 + SFA;
    }
    
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

    if (C.marikoEmcMaxClock <= 1862400) {
        std::memcpy(new_start, default_end, sizeof(emc_dvb_dvfs_table_t));
    } else if (C.marikoEmcMaxClock <= 2131200){
        emc_dvb_dvfs_table_t oc_table = { 2131200, { 700, 675, 650, } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    } else {
        emc_dvb_dvfs_table_t oc_table = { 2400000, { 730, 705, 680, } };
        std::memcpy(new_start, &oc_table, sizeof(emc_dvb_dvfs_table_t));
    }
    new_start->freq = C.marikoEmcMaxClock;

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
        { "GPU Freq PLL",   &GpuFreqPllLimit,       1, nullptr, GpuClkPllLimit },
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