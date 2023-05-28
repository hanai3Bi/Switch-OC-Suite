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
const std::array<u32, 6> tRAS_values = {42, 39, 36, 35, 34, 33};

const std::array<double, 5> tRRD_values = {10, 7.5, 6, 4, 2};
const std::array<double, 5> tFAW_values = {40, 30, 24, 16, 8};
const std::array<double, 5> tRTP_values = {7.5, 7.5, 6, 6, 5};

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

    if (C.mtcConf != AUTO_ADJ_SAFE_MARIKO && C.mtcConf != AUTO_ADJ_PERF_MARIKO)
    	return;

    const u32 i = C.mtcConf ? 0.65 : 1;

    #define ADJUST_PROP(TARGET, REF)                                                                        \
        (u32)(std::ceil( i * (REF + ((C.marikoEmcMaxClock-EmcClkOSAlt)*(TARGET-REF))/(EmcClkOSLimit-EmcClkOSAlt))))

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

    ADJUST_PARAM_ALL_REG(table, emc_r2w, ref); //0x140 0x4f0 0x880
    ADJUST_PARAM_ALL_REG(table, emc_w2r, ref); //0x144 0x4f4 0x884
    //ADJUST_PARAM_ALL_REG(table, emc_r2p, ref); //0x148 0x4f8 0x888
    //ADJUST_PARAM_ALL_REG(table, emc_w2p, ref); //0x14c 0x4fc 0x88c
    ADJUST_PARAM_ALL_REG(table, emc_trtm, ref); //0x158 0x508 0x898
    ADJUST_PARAM_ALL_REG(table, emc_twtm, ref); //0x15c 0x50c 0x89c
    ADJUST_PARAM_ALL_REG(table, emc_tratm, ref); //0x160 0x510 0x8a0
    ADJUST_PARAM_ALL_REG(table, emc_twatm, ref); //0x164 0x514 0x8a4

    ADJUST_PARAM_ALL_REG(table, emc_rw2pden, ref); //0x1fc 0x5ac 0x93c

    ADJUST_PARAM_ALL_REG(table, emc_tclkstop, ref); //0x22c 0x5dc 0x96c

    ADJUST_PARAM_ALL_REG(table, emc_pmacro_dll_cfg_2, ref); // EMC_DLL_CFG_2_0: level select for VDDA? //0x380 0x730 0xac0

    ADJUST_PARAM_TABLE(table, la_scale_regs.mc_mll_mpcorer_ptsa_rate, ref); //0xfa4
    ADJUST_PARAM_TABLE(table, la_scale_regs.mc_ptsa_grant_decrement, ref); //0xfac

    #define MAX(A, B)   std::max(A, B)
    #define MIN(A, B)   std::min(A, B)

    /* Timings that are available in or can be derived from LPDDR4X datasheet or TRM */
    
    const u32 TIMING_PRIM_PRESET = C.ramTimingPresetOne;
    const u32 TIMING_SECOND_PRESET = C.ramTimingPresetTwo;
    // tCK_avg (average clock period) in ns
    const double tCK_avg = 1000'000. / C.marikoEmcMaxClock;
    // tRPpb (row precharge time per bank) in ns
    const u32 tRPpb = !TIMING_PRIM_PRESET ? 18 : tRP_values[TIMING_PRIM_PRESET-1];
    // tRPab (row precharge time all banks) in ns
    const u32 tRPab = !TIMING_PRIM_PRESET ? 21 : tRPpb + 3;
    // tRAS (row active time) in ns
    const u32 tRAS = !TIMING_PRIM_PRESET ? 42 : tRAS_values[TIMING_PRIM_PRESET-1];
    // tRC (ACTIVATE-ACTIVATE command period same bank) in ns
    const u32 tRC = tRPpb + tRAS;
    // tRFCab (refresh cycle time all banks) in ns for 8Gb density
    const u32 tRFCab = 280;
    // tRFCpb (refresh cycle time per bank) in ns for 8Gb density
    const u32 tRFCpb = 140;
    // tRCD (RAS-CAS delay) in ns
    const u32 tRCD = !TIMING_PRIM_PRESET ? 18 : tRCD_values[TIMING_PRIM_PRESET-1];
    // tRRD (Active bank-A to Active bank-B) in ns
    const double tRRD = !TIMING_SECOND_PRESET ? 10. : tRRD_values[TIMING_SECOND_PRESET-1];
    // tREFpb (average refresh interval per bank) in ns for 8Gb density
    const u32 tREFpb = 488;
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
    // [Guessed] tACT2PDEN (timing delay from an activate, MRS or EMRS command to power-down entry) in ns
    const u32 tACT2PDEN = 14;
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
    const double tCKE = 7.5;
    // Delay from valid command to CKE input LOW in ns
    const double tCMDCKE = MAX(1.75, 3*tCK_avg);
    // tCKELPD (minimum CKE low pulse width in SELF REFRESH) in ns
    const u32 tCKELPD = 15;
    // [Guessed] tPD (minimum CKE low pulse width in power-down mode) in ns
    const double tPD = 7.5;
    // tFAW (Four-bank Activate Window) in ns
    const u32 tFAW = !TIMING_SECOND_PRESET ? 40 : tFAW_values[TIMING_SECOND_PRESET-1];

    // Internal READ-to-PRE-CHARGE command delay in ns
    const double tRTP = !TIMING_SECOND_PRESET ? 7.5 : tRTP_values[TIMING_SECOND_PRESET-1];
    const u32 WL = 10;
    const u32 BL = 16;
    // write-to-precharge time for commands to the same bank in cycles
    const double WTP = WL + BL/2 + 1 + std::ceil(18/tCK_avg);

    // Valid Clock requirement before CKE Input HIGH in ns
    const double tCKCKEH = MAX(1.75, 3*tCK_avg);

    #define GET_CYCLE_CEIL(PARAM) C.mtcConf ? u32(std::ceil(double(PARAM) / (1.2* tCK_avg))) : u32(std::ceil(double(PARAM) / tCK_avg))

    WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC)); //0x124
    WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab)); //0x128
    WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb)); //0x12c
    WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS)); //0x138
    WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb)); //0x13c
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
    WRITE_PARAM_ALL_REG(table, emc_act2pden,GET_CYCLE_CEIL(tACT2PDEN)); //0x1f4
    WRITE_PARAM_ALL_REG(table, emc_ar2pden, GET_CYCLE_CEIL(tCMDCKE));
    //WRITE_PARAM_ALL_REG(table, emc_rw2pden, RTPDEN);
    WRITE_PARAM_ALL_REG(table, emc_txsr,    MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe)); //0x20c
    WRITE_PARAM_ALL_REG(table, emc_txsrdll, MIN(GET_CYCLE_CEIL(tXSR), (u32)0x3fe)); //0x210
    WRITE_PARAM_ALL_REG(table, emc_tcke,    GET_CYCLE_CEIL(tCKE)); //0x214
    WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW)); //0x220
    WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab)); //0x224
    WRITE_PARAM_ALL_REG(table, emc_tclkstable, GET_CYCLE_CEIL(tCKCKEH));
    WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFRESH + 64); //0x234
    WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR)); //0x208
    WRITE_PARAM_ALL_REG(table, emc_cke2pden,GET_CYCLE_CEIL(tCKE2PDEN)); //0x200
    WRITE_PARAM_ALL_REG(table, emc_tckesr,  GET_CYCLE_CEIL(tCKELPD)); //0x218
    WRITE_PARAM_ALL_REG(table, emc_tpd,     GET_CYCLE_CEIL(tPD)); //0x21c
    
    constexpr u32 MC_ARB_DIV = 4; // Guessed
    constexpr u32 SFA = 2; // Guessed
    table->burst_mc_regs.mc_emem_arb_timing_rcd     = std::ceil(GET_CYCLE_CEIL(tRCD) / MC_ARB_DIV) - 2; //0xf30
    table->burst_mc_regs.mc_emem_arb_timing_rp      = std::ceil(GET_CYCLE_CEIL(tRPpb) / MC_ARB_DIV) - 1 + SFA; //0xf34
    table->burst_mc_regs.mc_emem_arb_timing_rc      = std::ceil(std::max(GET_CYCLE_CEIL(tRC), GET_CYCLE_CEIL(tRAS)+GET_CYCLE_CEIL(tRPpb)) / MC_ARB_DIV) - 1; //0xf38
    table->burst_mc_regs.mc_emem_arb_timing_ras     = std::ceil(GET_CYCLE_CEIL(tRAS) / MC_ARB_DIV) - 2; //0xf3c
    table->burst_mc_regs.mc_emem_arb_timing_faw     = std::ceil(GET_CYCLE_CEIL(tFAW) / MC_ARB_DIV) - 1; //0xf40
    table->burst_mc_regs.mc_emem_arb_timing_rrd     = std::ceil(GET_CYCLE_CEIL(tRRD) / MC_ARB_DIV) - 1; //0xf44
    table->burst_mc_regs.mc_emem_arb_timing_rap2pre = std::ceil(table->burst_regs.emc_r2p / MC_ARB_DIV); //0xf48
    table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(table->burst_regs.emc_w2p / MC_ARB_DIV); //0xf4c
    //table->burst_mc_regs.mc_emem_arb_timing_r2r     = std::ceil(table->burst_regs.emc_rext / MC_ARB_DIV) - 1 + SFA;
    //table->burst_mc_regs.mc_emem_arb_timing_w2w     = std::ceil(table->burst_regs.emc_wext / MC_ARB_DIV) - 1 + SFA;
    table->burst_mc_regs.mc_emem_arb_timing_r2w     = std::ceil(table->burst_regs.emc_r2w / MC_ARB_DIV) - 1 + SFA; //0xf58
    table->burst_mc_regs.mc_emem_arb_timing_w2r     = std::ceil(table->burst_regs.emc_w2r / MC_ARB_DIV) - 1 + SFA; //0xf60
    table->burst_mc_regs.mc_emem_arb_timing_rfcpb   = std::ceil(GET_CYCLE_CEIL(tRFCpb) / MC_ARB_DIV); //0xf64
    //table->burst_mc_regs.mc_emem_arb_timing_ccdmw   = std::ceil(tCCDMW / MC_ARB_DIV) -1 + SFA;
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
    MemMtcPllmbDivisor(table_max);
    // Overwrite 13312000 table with unmodified 1600000 table copied back
    std::memcpy(reinterpret_cast<void *>(table_alt), reinterpret_cast<void *>(tmp), sizeof(MarikoMtcTable));

    delete tmp;

    PATCH_OFFSET(ptr, C.marikoEmcMaxClock);

    // Handle customize table replacement
    if (C.mtcConf == CUSTOMIZED_ALL) {
        MemMtcCustomizeTable(table_list[0], reinterpret_cast<MarikoMtcTable *>(reinterpret_cast<u8 *>(C.marikoMtcTable)));
    }

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