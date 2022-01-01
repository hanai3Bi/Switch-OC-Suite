//#define EXPERIMENTAL
constexpr ro::ModuleId PcvModuleId[] = {
    // ParseModuleId("91D61D59D7002378E35584FC0B38C7693A3ABAB5"), //11.0.0
    // ParseModuleId("C503E96550F302E121873136B814A529863D949B"), //12.x
    ParseModuleId("2058C97C551571506656AA04EC85E2B1B01B155C"), //13.0.0-13.2.0
};

constexpr ro::ModuleId PtmModuleId[] = {
    // ParseModuleId("A79706954C6C45568B0FFE610627E2E89D8FB0D4"), //12.x
    ParseModuleId("2CA78D4066C1C11317CC2705EBADA9A51D3AC981"), //13.0.0-13.2.0
};

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
        u64 freq = 0;
        cvb_coefficients cvb_dfll_param;
        cvb_coefficients cvb_pll_param;  // only c0 is reserved
    } cpu_freq_cvb_table_t;

    typedef struct {
        u64 freq = 0;
        cvb_coefficients cvb_dfll_param; // empty, dfll clock source not selected
        cvb_coefficients cvb_pll_param;
    } gpu_cvb_pll_table_t;

    typedef struct {
        u64 freq;
        s32 volt[4] = {0};
    } emc_dvb_dvfs_table_t;

    /* CPU */
    constexpr u32 CpuVoltageLimitOffsets[][11] = {
        // { 0xE1A8C, 0xE1A98, 0xE1AA4, 0xE1AB0, 0xE1AF8, 0xE1B04, 0xE1B10, 0xE1B1C, 0xE1B28, 0xE1B34, 0xE1F4C },
        // { 0xF08DC, 0xF08E8, 0xF08F4, 0xF0900, 0xF0948, 0xF0954, 0xF0960, 0xF096C, 0xF0978, 0xF0984, 0xF0D9C },
        { 0xF092C, 0xF0938, 0xF0944, 0xF0950, 0xF0998, 0xF09A4, 0xF09B0, 0xF09BC, 0xF09C8, 0xF09D4, 0xF0DEC },
    };
    constexpr u32 NewCpuVoltageLimit = 1220;
    static_assert(NewCpuVoltageLimit <= 1300); //1300mV hangs for me

    constexpr u32 CpuVoltageOldTableCoeff[][10] = {
        // { 0xE2140, 0xE2178, 0xE21B0, 0xE21E8, 0xE2220, 0xE2258, 0xE2290, 0xE22C8, 0xE2300, 0xE2338 },
        // { 0xF0F90, 0xF0FC8, 0xF1000, 0xF1038, 0xF1070, 0xF10A8, 0xF10E0, 0xF1118, 0xF1150, 0xF1188 },
        { 0xF0FE0, 0xF1018, 0xF1050, 0xF1088, 0xF10C0, 0xF10F8, 0xF1130, 0xF1168, 0xF11A0, 0xF11D8 },
    };
    constexpr u32 CpuVoltageScale = 1000;
    constexpr u32 NewCpuVoltageScaled = NewCpuVoltageLimit * CpuVoltageScale;

    constexpr u32 CpuTablesFreeSpace[] = {
        // 0xE2350,
        // 0xF11A0,
        0xF11F0,
    };
    constexpr cpu_freq_cvb_table_t NewCpuTables[] = {
     // OldCpuTables
     // {  204000, {  721589, -12695, 27 }, { 1120000 } },
     // {  306000, {  747134, -14195, 27 }, { 1120000 } },
     // {  408000, {  776324, -15705, 27 }, { 1120000 } },
     // {  510000, {  809160, -17205, 27 }, { 1120000 } },
     // {  612000, {  845641, -18715, 27 }, { 1120000 } },
     // {  714000, {  885768, -20215, 27 }, { 1120000 } },
     // {  816000, {  929540, -21725, 27 }, { 1120000 } },
     // {  918000, {  976958, -23225, 27 }, { 1120000 } },
     // { 1020000, { 1028021, -24725, 27 }, { 1120000 } },
     // { 1122000, { 1082730, -26235, 27 }, { 1120000 } },
     // { 1224000, { 1141084, -27735, 27 }, { 1120000 } },
     // { 1326000, { 1203084, -29245, 27 }, { 1120000 } },
     // { 1428000, { 1268729, -30745, 27 }, { 1120000 } },
     // { 1581000, { 1374032, -33005, 27 }, { 1120000 } },
     // { 1683000, { 1448791, -34505, 27 }, { 1120000 } },
     // { 1785000, { 1527196, -36015, 27 }, { 1120000 } },
     // { 1887000, { 1609246, -37515, 27 }, { 1120000 } },
     // { 1963500, { 1675751, -38635, 27 }, { 1120000 } },
        { 2091000, { 1785520, -40523, 27 }, { NewCpuVoltageScaled } },
        { 2193000, { 1878755, -42027, 27 }, { NewCpuVoltageScaled } },
        { 2295000, { 1975655, -43531, 27 }, { NewCpuVoltageScaled } },
        { 2397000, { 2076220, -45036, 27 }, { NewCpuVoltageScaled } },
    };
    static_assert(sizeof(NewCpuTables) <= sizeof(cpu_freq_cvb_table_t)*14);

    constexpr u32 MaxCpuClockOffset[] = {
        // 0xE2740,
        // 0xF1590,
        0xF15E0,
    };
    constexpr u32 NewMaxCpuClock = 2397000;

    /* GPU */
    constexpr u32 GpuVoltageLimitOffsets[] = {
        // 0xE3044,
        // 0xF1E94,
        0xF1EE4,
    };
    constexpr u32 NewGpuVoltageLimit = 1170; // default max 1050mV

    constexpr u32 GpuTablesFreeSpace[] = {
        // 0xE3410,
        // 0xF2260,
        0xF22B0,
    };
    // No way to correctly derive c0-c5 coefficients, as coefficients >= 1152000 only make sense
    constexpr gpu_cvb_pll_table_t NewGpuTables[] = {
     // OldGpuTables
     // {  537600, {}, {  801688, -10900, -163,  298, -10599, 162 } },
     // {  614400, {}, {  824214,  -5743, -452,  238,  -6325,  81 } },
     // {  691200, {}, {  848830,  -3903, -552,  119,  -4030,  -2 } },
     // {  768000, {}, {  891575,  -4409, -584,    0,  -2849,  39 } },
     // {  844800, {}, {  940071,  -5367, -602,  -60,    -63, -93 } },
     // {  921600, {}, {  986765,  -6637, -614, -179,   1905, -13 } },
     // {  998400, {}, { 1098475, -13529, -497, -179,   3626,   9 } },
     // { 1075200, {}, { 1163644, -12688, -648,    0,   1077,  40 } },
     // { 1152000, {}, { 1204812,  -9908, -830,    0,   1469, 110 } },
     // { 1228800, {}, { 1277303, -11675, -859,    0,   3722, 313 } },
     // { 1267200, {}, { 1335531, -12567, -867,    0,   3681, 559 } },
        { 1305600, {}, {  955000 } },
        { 1344000, {}, {  990000 } },
        { 1382400, {}, { 1030000 } },
        { 1420800, {}, { 1075000 } },
        { 1459200, {}, { 1120000 } },
        { 1497600, {}, { 1170000 } },
     // { 1536000, {}, { 1250000 } },
    };
    static_assert(sizeof(NewGpuTables) <= sizeof(gpu_cvb_pll_table_t)*15);

    constexpr u32 Reg1MaxGpuOffset[] = {
        // 0x2E0AC,
        // 0x3F6CC,
        0x3F12C,
    };
    constexpr u8 Reg1NewMaxGpuClock[][0xC] = {
        /* Original: 1228.8MHz
         *
         * MOV   W13,#0x1000
         * MOVK  W13,#0xE,LSL #16
         * ADD   X13, X13, #0x4B,LSL#12
         *
         * Bump to 1536MHz
         *
         * MOV   W13,#0x7000
         * MOVK  W13,#0x17,LSL #16
         * NOP
         */
        // { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
        // { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
    };

    constexpr u32 Reg2MaxGpuOffset[] = {
        // 0x2E110,
        // 0x3F730,
        0x3F190,
    };
    constexpr u8 Reg2NewMaxGpuClock[][0x8] = {
        /* Original: 921.6MHz
         *
         * MOV   W13,#0x1000
         * MOVK  W13,#0xE,LSL #16
         *
         * Bump to 1536MHz
         *
         * MOV   W13,#0x7000
         * MOVK  W13,#0x17,LSL #16
         */
        // { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, },
        // { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, },
    };

    /* EMC */

    // DvbTable is all about frequency scaling along with CPU core voltage, no need to care about this for now.

    // constexpr u32 EmcDvbTableOffsets[] =
    // {
    //     0xFFFFFFFF,
    //     0xFFFFFFFF,
    //     0xF0628,
    // };

    // constexpr emc_dvb_dvfs_table_t EmcDvbTable[6] =
    // {
    //     {  204000, {  637,  637,  637, } },
    //     {  408000, {  637,  637,  637, } },
    //     {  800000, {  637,  637,  637, } },
    //     { 1065600, {  637,  637,  637, } },
    //     { 1331200, {  650,  637,  637, } },
    //     { 1600000, {  675,  650,  637, } },
    // };

    constexpr u32 EmcDvb1331[] = {
        0xF0688,
    };

    // Sourced from 13.x pcv module
    // 1st regulator table, 0x142778 - 0x143BB4, if mask = 0b0110101
    // 2nd regulator table, 0x143BB8 - 0x144FF4, if mask = 0b1010011

    // Access pattern:
    // BL          0x6C390         // read mask from 0x195588 (.bss section) and return X0 (address of regulator table)
    // MOV         W8, #0x120      // offset per entry
    // (S)MADD(L)  X8, X22, X8, X0 // X8 = X22 * X8 + X0, X22 is regulator entry ID (0x11 for max77812_dram)
    // LDR         W8, [X8, #0x10] // read maxim regulator identifier
    // CMP         W8, #3
    // B.EQ        ...

    // 1st regulator table:
    // 0x143A98    2               #0x0
    // 0x143A9C    0               #0x4
    // 0x143AA0    "max77812_dram" #0x8
    // 0x143AA8    3               #0x10 // maxim regulator identifier ( 1 = max77620, 2 = max77621, 3 = max77812)
    // 0x143AAC    0               #0x14
    // 0x143AB0    5000            #0x18 // voltage step
    // 0x143AB4    0               #0x1C
    // 0x143AB8    250000          #0x20 // min voltage
    // 0x143ABC    1525000         #0x24 // max voltage
    // 0x143AC0    0               #0x28 // voltage multiplier ( * step )
    // 0x143AC4    600000          #0x2C

    // 0x142898    1               #0x0
    // 0x14289C    0               #0x4
    // 0x1428A0    "max77620_sd1"  #0x8
    // 0x1428A8    1               #0x10 // maxim regulator identifier ( 1 = max77620, 2 = max77621, 3 = max77812)
    // 0x1428AC    23              #0x14
    // 0x1428B0    12500           #0x18 // voltage step
    // 0x1428B4    600000          #0x1C
    // 0x1428B8    1125000         #0x20 // min voltage, default Vddq for Erista EMC
    // 0x1428BC    1125000         #0x24 // max voltage, default Vddq for Erista EMC
    // 0x1428C0    0               #0x28
    // 0x1428C4    0               #0x2C

    // HOS does not seem to change DRAM voltage on Mariko (validate only)

    // void EnableVddMemory() in Atmosphere/libraries/libexosphere/source/pmic/pmic_api.cpp:
    // /* On Erista, set Sd1 voltage. */
    // if (soc_type == fuse::SocType_Erista) {
    //     SetVoltage(Max77620RegisterSd1, 1100);
    // }

    // in hekate/bdk/power/max77812.h:
    // #define MAX77812_REG_M3_VOUT     0x25 // DRAM on PHASE211.
    // 3 outputs (CPU/GPU/DRAM) from max77812. Does PHASE31 mode exist?
    // If so, read/query max77812 pmic via i2c for voltage info in hekate and get DRAM reg on PHASE31.
    // max77812 document: https://datasheets.maximintegrated.com/en/ds/MAX77812.pdf

    // Mariko have 3 mtc tables (204/1331/1600 MHz), only these 3 frequencies could be set.
    constexpr u32 EmcFreqOffsets[][30] = {
        // { 0xD7C60, 0xD7C68, 0xD7C70, 0xD7C78, 0xD7C80, 0xD7C88, 0xD7C90, 0xD7C98, 0xD7CA0, 0xD7CA8, 0xE1800, 0xEEFA0, 0xF2478, 0xFE284, 0x10A304, 0x10D7DC, 0x110A40, 0x113CA4, 0x116F08, 0x11A16C, 0x11D3D0, 0x120634, 0x123898, 0x126AFC, 0x129D60, 0x12CFC4, 0x130228, 0x13BFE0, 0x140D00, 0x140D50, },
        // { 0xE1810, 0xE6530, 0xE6580, 0xE6AB0, 0xE6AB8, 0xE6AC0, 0xE6AC8, 0xE6AD0, 0xE6AD8, 0xE6AE0, 0xE6AE8, 0xE6AF0, 0xE6AF8, 0xF0650, 0xFDDF0, 0x1012C8, 0x10D0D4, 0x119154, 0x11C62C, 0x11F890, 0x122AF4, 0x125D58, 0x128FBC, 0x12C220, 0x12F484, 0x1326E8, 0x13594C, 0x138BB0, 0x13BE14, 0x13F078, },
        { 0xE1860, 0xE6580, 0xE65D0, 0xE6B00, 0xE6B08, 0xE6B10, 0xE6B18, 0xE6B20, 0xE6B28, 0xE6B30, 0xE6B38, 0xE6B40, 0xE6B48, 0xF06A0, 0xFDE40, 0x101318, 0x10D124, 0x1191A4, 0x11C67C, 0x11F8E0, 0x122B44, 0x125DA8, 0x12900C, 0x12C270, 0x12F4D4, 0x132738, 0x13599C, 0x138C00, 0x13BE64, 0x13F0C8, },
    };

    // Mariko mtc tables starting from rev, see mtc_timing_table.hpp for parameters.
    // All mariko mtc tables will be patched to simplify the procedure.
    constexpr u32 MtcTable_1600[][13] = {
        { 0x1012D8, 0x11C63C, 0x11F8A0, 0x122B04, 0x125D68, 0x128FCC, 0x12C230, 0x12F494, 0x1326F8, 0x13595C, 0x138BC0, 0x13BE24, 0x13F088 },
    };

    constexpr u32 MtcTableOffset = 0x10CC;

    #include "mtc_timing_table.hpp"

    void AdjustMtcTable(MarikoMtcTable* table, MarikoMtcTable* ref)
    {
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

        #define ADJUST_PARAM(TARGET, REF)                TARGET = std::ceil(REF + ((GetEmcClock()-1331200)*(TARGET-REF))/(1600000-1331200));

        #define ADJUST_PARAM_TABLE(TABLE, PARAM, REF)    ADJUST_PARAM(TABLE->PARAM, REF->PARAM)

        #define ADJUST_PARAM_ALL_REG(TABLE, PARAM, REF)                 \
            ADJUST_PARAM_TABLE(TABLE, burst_regs.PARAM, REF)            \
            ADJUST_PARAM_TABLE(TABLE, shadow_regs_ca_train.PARAM, REF)  \
            ADJUST_PARAM_TABLE(TABLE, shadow_regs_rdwr_train.PARAM, REF)

        #define WRITE_PARAM_ALL_REG(TABLE, PARAM, VALUE)\
            TABLE->burst_regs.PARAM = VALUE;            \
            TABLE->shadow_regs_ca_train.PARAM = VALUE;  \
            TABLE->shadow_regs_rdwr_train.PARAM = VALUE;

        // tCK_avg (average clock period) in ns (10E-3 ns)
        const double tCK_avg = GetEmcClock() == 2131200 ? 0.468 : 1000'000. / GetEmcClock();
        // tRPpb (row precharge time per bank) in ns
        const u32 tRPpb = 18;
        // tRPab (row precharge time all banks) in ns
        const u32 tRPab = 21;
        // tRAS (row active time) in ns
        const u32 tRAS = 42;
        // tRC (ACTIVATE-ACTIVATE command period same bank) in ns
        const u32 tRC = tRPpb + tRAS;
        // tRFCab (refresh cycle time all banks) in ns for 8Gb density
        const u32 tRFCab = 280;
        // tRFCpb (refresh cycle time per bank) in ns for 8Gb density
        const u32 tRFCpb = 140;
        // tRCD (RAS-CAS delay) in ns
        const u32 tRCD = 18;
        // tRRD (Active bank-A to Active bank-B) in ns
        const double tRRD = GetEmcClock() == 2131200 ? 7.5 : 10.;
        // tREFpb (average refresh interval per bank) in ns for 8Gb density
        const u32 tREFpb = 488;
        // tREFab (average refresh interval all 8 banks) in ns for 8Gb density
        // const u32 tREFab = tREFpb * 8;
        // #_of_rows per die for 8Gb density
        const u32 numOfRows = 65536;
        // {REFRESH, REFRESH_LO} = max[(tREF/#_of_rows) / (emc_clk_period) - 64, (tREF/#_of_rows) / (emc_clk_period) * 97%]
        // emc_clk_period = dram_clk / 2;
        // 1600 MHz: 5894, but N' set to 6176 (~4.8% margin)
        const u32 REFRESH = std::ceil((double(tREFpb) * GetEmcClock() / numOfRows * (1.048) / 2 - 64)) / 4 * 4;
        // tPDEX2WR, tPDEX2RD (timing delay from exiting powerdown mode to a write/read command) in ns
        const u32 tPDEX2 = 10;
        // [Guessed] tACT2PDEN (timing delay from an activate, MRS or EMRS command to power-down entry) in ns
        const u32 tACT2PDEN = 14;
        // [Guessed] tPDEX2MRR (timing delay from exiting powerdown mode to MRR command) in ns
        const double tPDEX2MRR = 28.75;
        // [Guessed] tCKE2PDEN (timing delay from turning off CKE to power-down entry) in ns
        const double tCKE2PDEN = 8.5;
        // tXSR (SELF REFRESH exit to next valid command delay) in ns
        const double tXSR = tRFCab + 7.5;
        // tCKE (minimum CKE high pulse width) in ns
        const u32 tCKE = 8;
        // tCKELPD (minimum CKE low pulse width in SELF REFRESH) in ns
        const u32 tCKELPD = 15;
        // [Guessed] tPD (minimum CKE low pulse width in power-down mode) in ns
        const double tPD = 7.5;
        // tFAW (Four-bank Activate Window) in ns
        const u32 tFAW = GetEmcClock() == 2131200 ? 30 : 40;

        #define GET_CYCLE_CEIL(PARAM) std::ceil(double(PARAM) / tCK_avg)

        WRITE_PARAM_ALL_REG(table, emc_rc,      GET_CYCLE_CEIL(tRC));
        WRITE_PARAM_ALL_REG(table, emc_rfc,     GET_CYCLE_CEIL(tRFCab));
        WRITE_PARAM_ALL_REG(table, emc_rfcpb,   GET_CYCLE_CEIL(tRFCpb));
        WRITE_PARAM_ALL_REG(table, emc_ras,     GET_CYCLE_CEIL(tRAS));
        WRITE_PARAM_ALL_REG(table, emc_rp,      GET_CYCLE_CEIL(tRPpb));
        ADJUST_PARAM_ALL_REG(table, emc_r2w, ref);
        ADJUST_PARAM_ALL_REG(table, emc_w2r, ref);
        ADJUST_PARAM_ALL_REG(table, emc_r2p, ref);
        ADJUST_PARAM_ALL_REG(table, emc_w2p, ref);
        ADJUST_PARAM_ALL_REG(table, emc_trtm, ref);
        ADJUST_PARAM_ALL_REG(table, emc_twtm, ref);
        ADJUST_PARAM_ALL_REG(table, emc_tratm, ref);
        ADJUST_PARAM_ALL_REG(table, emc_twatm, ref);
        WRITE_PARAM_ALL_REG(table, emc_rd_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_wr_rcd,  GET_CYCLE_CEIL(tRCD));
        WRITE_PARAM_ALL_REG(table, emc_rrd,     GET_CYCLE_CEIL(tRRD));
        WRITE_PARAM_ALL_REG(table, emc_refresh, REFRESH);
        WRITE_PARAM_ALL_REG(table, emc_pre_refresh_req_cnt, REFRESH / 4);
        WRITE_PARAM_ALL_REG(table, emc_pdex2wr, GET_CYCLE_CEIL(tPDEX2));
        WRITE_PARAM_ALL_REG(table, emc_pdex2rd, GET_CYCLE_CEIL(tPDEX2));
        WRITE_PARAM_ALL_REG(table, emc_act2pden,GET_CYCLE_CEIL(tACT2PDEN));
        ADJUST_PARAM_ALL_REG(table, emc_rw2pden, ref);
        WRITE_PARAM_ALL_REG(table, emc_cke2pden,GET_CYCLE_CEIL(tCKE2PDEN));
        WRITE_PARAM_ALL_REG(table, emc_pdex2mrr,GET_CYCLE_CEIL(tPDEX2MRR));
        WRITE_PARAM_ALL_REG(table, emc_txsr,    GET_CYCLE_CEIL(tXSR));
        WRITE_PARAM_ALL_REG(table, emc_txsrdll, GET_CYCLE_CEIL(tXSR));
        WRITE_PARAM_ALL_REG(table, emc_tcke,    GET_CYCLE_CEIL(tCKE));
        WRITE_PARAM_ALL_REG(table, emc_tckesr,  GET_CYCLE_CEIL(tCKELPD));
        WRITE_PARAM_ALL_REG(table, emc_tpd,     GET_CYCLE_CEIL(tPD));
        WRITE_PARAM_ALL_REG(table, emc_tfaw,    GET_CYCLE_CEIL(tFAW));
        WRITE_PARAM_ALL_REG(table, emc_trpab,   GET_CYCLE_CEIL(tRPab));
        ADJUST_PARAM_ALL_REG(table, emc_tclkstop, ref);
        WRITE_PARAM_ALL_REG(table, emc_trefbw,  REFRESH + 64);

        ADJUST_PARAM_ALL_REG(table, emc_pmacro_dll_cfg_2, ref); // EMC_DLL_CFG_2_0: level select for VDDA?

        // ADJUST_PARAM_TABLE(table, dram_timings.rl); // not used on Mariko

        constexpr u32 DIV = 4; // ?
        table->burst_mc_regs.mc_emem_arb_timing_rcd = std::ceil(GET_CYCLE_CEIL(tRCD) / DIV - 2);
        table->burst_mc_regs.mc_emem_arb_timing_rp  = std::ceil(GET_CYCLE_CEIL(tRPpb) / DIV - 1);
        table->burst_mc_regs.mc_emem_arb_timing_rc  = std::ceil(std::max(GET_CYCLE_CEIL(tRC), GET_CYCLE_CEIL(tRAS)+GET_CYCLE_CEIL(tRPpb))/ DIV);
        table->burst_mc_regs.mc_emem_arb_timing_ras = std::ceil(GET_CYCLE_CEIL(tRAS) / DIV - 2);
        table->burst_mc_regs.mc_emem_arb_timing_faw = std::ceil(GET_CYCLE_CEIL(tFAW) / DIV - 1);
        table->burst_mc_regs.mc_emem_arb_timing_rrd = std::ceil(GET_CYCLE_CEIL(tRRD) / DIV - 1);
        table->burst_mc_regs.mc_emem_arb_timing_rap2pre = std::ceil(table->burst_regs.emc_r2p / DIV);
        table->burst_mc_regs.mc_emem_arb_timing_wap2pre = std::ceil(table->burst_regs.emc_w2p / DIV);
        table->burst_mc_regs.mc_emem_arb_timing_r2w = std::ceil(table->burst_regs.emc_r2w / DIV + 1);
        table->burst_mc_regs.mc_emem_arb_timing_w2r = std::ceil(table->burst_regs.emc_w2r / DIV + 1);
        table->burst_mc_regs.mc_emem_arb_timing_rfcpb = std::ceil(GET_CYCLE_CEIL(tRFCpb) / DIV + 1); // ?

        ADJUST_PARAM_TABLE(table, la_scale_regs.mc_mll_mpcorer_ptsa_rate, ref);
        ADJUST_PARAM_TABLE(table, la_scale_regs.mc_ptsa_grant_decrement, ref);

        // ADJUST_PARAM_TABLE(table, min_mrs_wait); // not used on LPDDR4X
        // ADJUST_PARAM_TABLE(table, latency); // not used

        // Calculate DIVM and DIVN (clock DIVisors)
        // Common PLL oscillator is 38.4 MHz
        // PLLMB_OUT = 38.4 MHz / PLLLMB_DIVM * PLLMB_DIVN
        u32 divm = 1;
        u32 divn = GetEmcClock() / 38400;
        u32 remainder = GetEmcClock() % 38400;
        if (remainder >= 38400 * (3/4)) {
            divm = 4;
            divn = divn * divm + 3;
        } else
        if (remainder >= 38400 * (2/3)) {
            divm = 3;
            divn = divn * divm + 2;
        } else
        if (remainder >= 38400 * (1/2)) {
            divm = 2;
            divn = divn * divm + 1;
        } else
        if (remainder >= 38400 * (1/3)) {
            divm = 3;
            divn = divn * divm + 1;
        } else
        if (remainder >= 38400 * (1/4)) {
            divm = 4;
            divn = divn * divm + 1;
        }

        /* Patch PLLMB divisors */
        table->pllmb_divm = divm;
        table->pllmb_divn = divn;

        #ifdef EXPERIMENTAL
        {
            #define ADJUST_PARAM_ROUND2_ALL_REG(TARGET_TABLE, REF_TABLE, PARAM)                                                        \
                TARGET_TABLE->burst_regs.PARAM =                                                                                       \
                    ((ADJUST_PROP(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM) + 1) >> 1) << 1;                        \
                TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                             \
                    ((ADJUST_PROP(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM) + 1) >> 1) << 1;    \
                TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                           \
                    ((ADJUST_PROP(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM) + 1) >> 1) << 1;

            #define ADJUST_PARAM(TARGET_PARAM, REF_PARAM)           \
                TARGET_PARAM = ADJUST_PROP(TARGET_PARAM, REF_PARAM);

            #define ADJUST_PARAM_TABLE(TARGET_TABLE, REF_TABLE, PARAM) \
                ADJUST_PARAM(TARGET_TABLE->PARAM, REF_TABLE->PARAM)

            #define ADJUST_PARAM_ALL_REG(TARGET_TABLE, REF_TABLE, PARAM)                 \
                ADJUST_PARAM_TABLE(TARGET_TABLE, REF_TABLE, burst_regs.PARAM)            \
                ADJUST_PARAM_TABLE(TARGET_TABLE, REF_TABLE, shadow_regs_ca_train.PARAM)  \
                ADJUST_PARAM_TABLE(TARGET_TABLE, REF_TABLE, shadow_regs_rdwr_train.PARAM)

            #define TRIM_BIT(IN_BITS, HIGH, LOW)                       \
                ((IN_BITS >> LOW) & ( (1u << (HIGH - LOW + 1u)) - 1u ))

            #define ADJUST_BIT(TARGET_PARAM, REF_PARAM, HIGH, LOW)                            \
                ADJUST_PROP(TRIM_BIT(TARGET_PARAM, HIGH, LOW), TRIM_BIT(REF_PARAM, HIGH, LOW))

            #define CLEAR_BIT(BITS, HIGH, LOW)                        \
                BITS = BITS & ~( ((1u << HIGH) << 1u) - (1u << LOW) );

            #define ADJUST_BIT_ALL_REG_SINGLE_OP(TARGET_TABLE, REF_TABLE, PARAM, HIGH, LOW, OPERATION)                                            \
                TARGET_TABLE->burst_regs.PARAM =                                                                                                  \
                    (ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH, LOW) << LOW) OPERATION;                        \
                TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                                        \
                    (ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH, LOW)) << LOW OPERATION;    \
                TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                                      \
                    (ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH, LOW)) << LOW OPERATION;

            #define ADJUST_BIT_ALL_REG_PAIR(TARGET_TABLE, REF_TABLE, PARAM, HIGH1, LOW1, HIGH2, LOW2)                                      \
                TARGET_TABLE->burst_regs.PARAM =                                                                                           \
                    ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH1, LOW1) << LOW1                           \
                    | ADJUST_BIT(TARGET_TABLE->burst_regs.PARAM, REF_TABLE->burst_regs.PARAM, HIGH2, LOW2) << LOW2;                        \
                TARGET_TABLE->shadow_regs_ca_train.PARAM =                                                                                 \
                    ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH1, LOW1) << LOW1       \
                    | ADJUST_BIT(TARGET_TABLE->shadow_regs_ca_train.PARAM, REF_TABLE->shadow_regs_ca_train.PARAM, HIGH2, LOW2) << LOW2;    \
                TARGET_TABLE->shadow_regs_rdwr_train.PARAM =                                                                               \
                    ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH1, LOW1) << LOW1   \
                    | ADJUST_BIT(TARGET_TABLE->shadow_regs_rdwr_train.PARAM, REF_TABLE->shadow_regs_rdwr_train.PARAM, HIGH2, LOW2) << LOW2;

            /* For latency allowance */
            #define ADJUST_INVERSE(TARGET) ((TARGET*1000) / (GetEmcClock()/1600))

            /* emc_wdv, emc_wsv, emc_wev, emc_wdv_mask,
               emc_quse, emc_quse_width, emc_ibdly, emc_obdly,
               emc_einput, emc_einput_duration, emc_qrst, emc_qsafe,
               emc_rdv, emc_rdv_mask, emc_rdv_early, emc_rdv_early_mask */
            ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wdv);
            ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wsv);
            ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wev);
            ADJUST_PARAM_ROUND2_ALL_REG(target_table, ref_table, emc_wdv_mask);

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_quse);
            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_quse_width);

            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_ibdly, 6,0, | (1 << 28));
            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_obdly, 5,0, | (1 << 28));

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_einput);
            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_einput_duration);

            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_qrst, 6,0, | (6 << 16));
            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_qsafe);

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_rdv);
            target_table->burst_regs.emc_rdv_mask = target_table->burst_regs.emc_rdv + 2;
            target_table->shadow_regs_ca_train.emc_rdv_mask = target_table->shadow_regs_ca_train.emc_rdv + 2;
            target_table->shadow_regs_rdwr_train.emc_rdv_mask = target_table->shadow_regs_rdwr_train.emc_rdv + 2;

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_rdv_early);
            target_table->burst_regs.emc_rdv_early_mask = target_table->burst_regs.emc_rdv_early + 2;
            target_table->shadow_regs_ca_train.emc_rdv_early_mask = target_table->shadow_regs_ca_train.emc_rdv_early + 2;
            target_table->shadow_regs_rdwr_train.emc_rdv_early_mask = target_table->shadow_regs_rdwr_train.emc_rdv_early + 2;

            /* emc_pmacro_...,
               emc_zcal_wait_cnt, emc_mrs_wait_cnt(2),
               emc_pmacro_autocal_cfg_common, emc_dyn_self_ref_control, emc_qpop, emc_pmacro_cmd_pad_tx_ctrl,
               emc_tr_timing_0, emc_tr_rdv, emc_tr_qpop, emc_tr_rdv_mask, emc_tr_qsafe, emc_tr_qrst,
               emc_training_vref_settle */
            /* DDLL values */
            {
                #define OFFSET_ALL_REG(PARAM)                               \
                    offsetof(MarikoMtcTable, burst_regs.PARAM),             \
                    offsetof(MarikoMtcTable, shadow_regs_ca_train.PARAM),   \
                    offsetof(MarikoMtcTable, shadow_regs_rdwr_train.PARAM)  \

                /* Section 1: adjust HI bits: BIT 26:16 */
                const uint32_t ddll_high[] = {
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_4),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_5),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_4),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_5),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_4),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_5),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_0),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_1),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_2),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_3),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_4),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3),
                };
                for (uint32_t i = 0; i < sizeof(ddll_high)/sizeof(uint32_t); i++)
                {
                    uint32_t *ddll = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + ddll_high[i]);
                    uint32_t *ddll_ref = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(ref_table) + ddll_high[i]);
                    uint16_t adjusted_ddll = ADJUST_BIT(*ddll, *ddll_ref, 26,16) & ((1 << 10) - 1);
                    CLEAR_BIT(*ddll, 26,16)
                    *ddll |= adjusted_ddll << 16;
                }

                /* Section 2: adjust LOW bits: BIT 10:0 */
                const uint32_t ddll_low[] = {
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_4),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dq_rank1_5),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_0),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_1),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_3),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank0_4),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_0),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_1),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_3),
                    OFFSET_ALL_REG(emc_pmacro_ob_ddll_long_dqs_rank1_4),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_0),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_1),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_2),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_3),
                    OFFSET_ALL_REG(emc_pmacro_ddll_long_cmd_4),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2),
                    offsetof(MarikoMtcTable, trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3),
                };
                for (uint32_t i = 0; i < sizeof(ddll_low)/sizeof(uint32_t); i++)
                {
                    uint32_t *ddll = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + ddll_low[i]);
                    uint32_t *ddll_ref = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(ref_table) + ddll_low[i]);
                    uint16_t adjusted_ddll = ADJUST_BIT(*ddll, *ddll_ref, 10,0) & ((1 << 10) - 1);
                    CLEAR_BIT(*ddll, 10,0)
                    *ddll |= adjusted_ddll;
                }
            }

            ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_zcal_wait_cnt, 21,16, 10,0)
            ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_mrs_wait_cnt, 21,16, 10,0)
            ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_mrs_wait_cnt2, 21,16, 10,0)

            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_auto_cal_channel, 5,0, | 0xC1E00300)
            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_pmacro_autocal_cfg_common, 5,0, | 8 << 8)

            ADJUST_BIT_ALL_REG_PAIR(target_table, ref_table, emc_dyn_self_ref_control, 31,31, 15,0)

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_qpop);

            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_tr_timing_0, 9,0, | 0x1186100)

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_rdv);
            target_table->burst_regs.emc_tr_rdv_mask = target_table->burst_regs.emc_tr_rdv + 2;
            target_table->shadow_regs_ca_train.emc_tr_rdv_mask = target_table->shadow_regs_ca_train.emc_tr_rdv + 2;
            target_table->shadow_regs_rdwr_train.emc_tr_rdv_mask = target_table->shadow_regs_rdwr_train.emc_tr_rdv + 2;

            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_qpop);
            ADJUST_PARAM_ALL_REG(target_table, ref_table, emc_tr_qsafe);
            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_tr_qrst, 6,0, | (6 << 16));

            ADJUST_BIT_ALL_REG_SINGLE_OP(target_table, ref_table, emc_training_vref_settle, 15,0, | (4 << 16));

            /* External Memory Arbitration Configuration */
            /* BIT 20:16 - EXTRA_TICKS_PER_UPDATE: 0 */
            /* BIT 8:0   - CYCLES_PER_UPDATE: 12(1600MHz), 10(1331.2MHz) */
            ADJUST_PARAM_TABLE(target_table, ref_table, burst_mc_regs.mc_emem_arb_cfg);

            /* External Memory Arbitration Configuration: Direction Arbiter: Turns */
            /* BIT 31:24 - W2R_TURN: approx. mc_emem_arb_timing_w2r */
            /* BIT 23:16 - R2W_TURN: approx. mc_emem_arb_timing_r2w */
            /* BIT 15:8  - W2W_TURN: 0 */
            /* BIT 7:0   - R2R_TURN: 0 */
            {
                uint32_t param_1600 = target_table->burst_mc_regs.mc_emem_arb_da_turns;
                uint32_t param_1331 = ref_table->burst_mc_regs.mc_emem_arb_da_turns;
                uint8_t w2r_turn = ADJUST_BIT(param_1600, param_1331, 31,24);
                uint8_t r2w_turn = ADJUST_BIT(param_1600, param_1331, 23,16);
                target_table->burst_mc_regs.mc_emem_arb_da_turns = w2r_turn << 24 | r2w_turn << 16;
            }

            /* External Memory Arbitration Configuration: Direction Arbiter: Covers */
            /* BIT 23:16 - RCD_W_COVER: 13(1600MHz), 11(1331.2MHz) */
            /* BIT 15:8  - RCD_R_COVER: 8(1600MHz), 7(1331.2MHz) */
            /* BIT 7:0   - RC_COVER: approx. mc_emem_arb_timing_rc, 12(1600MHz), 9(1331.2MHz) */
            {
                uint32_t param_1600 = target_table->burst_mc_regs.mc_emem_arb_da_covers;
                uint32_t param_1331 = ref_table->burst_mc_regs.mc_emem_arb_da_covers;
                uint8_t rcd_w_cover = ADJUST_BIT(param_1600, param_1331, 23,16);
                uint8_t rcd_r_cover = ADJUST_BIT(param_1600, param_1331, 15,8);
                uint8_t rc_cover    = ADJUST_BIT(param_1600, param_1331, 7,0);
                target_table->burst_mc_regs.mc_emem_arb_da_covers = rcd_w_cover << 16 | rcd_r_cover << 8 | rc_cover;
            }

            /* External Memory Arbitration Configuration: Miscellaneous Thresholds (0) */
            /* BIT 20:16 - PRIORITY_INVERSION_ISO_THRESHOLD: 12(1600MHz), 10(1331.2MHz) */
            /* BIT 14:8  - PRIORITY_INVERSION_THRESHOLD: 36(1600MHz), 30(1331.2MHz) */
            /* BIT 7:0   - BC2AA_HOLDOFF_THRESHOLD: set to mc_emem_arb_timing_rc */
            {
                uint32_t param_1600 = target_table->burst_mc_regs.mc_emem_arb_misc0;
                uint32_t param_1331 = ref_table->burst_mc_regs.mc_emem_arb_misc0;
                uint8_t priority_inversion_iso_threshold = ADJUST_BIT(param_1600, param_1331, 20,16);
                uint8_t priority_inversion_threshold     = ADJUST_BIT(param_1600, param_1331, 14,8);
                uint8_t bc2aa_holdoff_threshold          = target_table->burst_mc_regs.mc_emem_arb_timing_rc;
                CLEAR_BIT(target_table->burst_mc_regs.mc_emem_arb_misc0, 20,16)
                CLEAR_BIT(target_table->burst_mc_regs.mc_emem_arb_misc0, 14,8)
                CLEAR_BIT(target_table->burst_mc_regs.mc_emem_arb_misc0, 7,0)
                target_table->burst_mc_regs.mc_emem_arb_misc0 |=
                    (priority_inversion_iso_threshold << 16 | priority_inversion_threshold << 8 | bc2aa_holdoff_threshold);
            }

            /* Latency allowance settings */
            {
                /* Section 1: adjust write latency */
                /* BIT 23:16 - ALLOWANCE_WRITE: 128(1600MHz), 153(1331.2MHz) */
                const uint32_t latency_write_offset[] = {
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_xusb_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_xusb_1),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_tsec_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmca_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmcaa_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmc_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_sdmmcab_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_ppcs_1),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_mpcore_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_avpc_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu2_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_nvenc_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_nvdec_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vic_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_isp2_1),
                };
                for (uint32_t i = 0; i < sizeof(latency_write_offset)/sizeof(uint32_t); i++)
                {
                    uint32_t *latency = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + latency_write_offset[i]);
                    CLEAR_BIT(*latency, 23,16)
                    *latency |= ADJUST_INVERSE(128) << 16;
                }

                /* Section 2: adjust read latency */
                /* BIT 7:0   - ALLOWANCE_READ */
                const uint32_t latency_read_offset[] = {
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_hc_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_hc_1),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_gpu2_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vic_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_vi2_0),
                    offsetof(MarikoMtcTable, la_scale_regs.mc_latency_allowance_isp2_1),
                };
                for (uint32_t i = 0; i < sizeof(latency_read_offset)/sizeof(uint32_t); i++)
                {
                    uint32_t *latency = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(target_table) + latency_read_offset[i]);
                    uint8_t adjusted_latency = ADJUST_INVERSE(TRIM_BIT(*latency, 7,0));
                    CLEAR_BIT(*latency, 7,0)
                    *latency |= adjusted_latency;
                }
            }

            /* PLLM and PLLMB control */
            {
                /*
                 * CLK_RST_CONTROLLER_PLLM_SS_CTRL1:
                 * BIT 31:16 : PLLM_SDM_SSC_MAX
                 * BIT 15:0  : PLLM_SDM_SSC_MIN
                 *
                 * CLK_RST_CONTROLLER_PLLM_SS_CTRL2:
                 * BIT 31:16 : PLLM_SDM_SSC_STEP
                 * BIT 15:0  : PLLM_SDM_DIN
                 *
                 * pllm(b)_ss_ctrl1:
                 *   1365,    342 (1600MHz)
                 * 0xFAAB, 0xF404 (1331MHz)
                 *
                 * pllm(b)_ss_ctrl2:
                 *      2,   1365 (1600MHz)
                 *      6, 0xFAAB (1331MHz)
                 *
                 * No need to care about this if Spread Spectrum (SS) is disabled
                 */
                // Disable PLL Spread Spectrum Control
                table->pll_en_ssc = 0;
                table->pllm_ss_cfg = 1 << 30;
            }

            /* EMC misc. configuration */
            {
                /* ? Command Trigger: MRW, MRW2: MRW_OP - [PMC] data to be written ?
                 *
                 *  EMC_MRW: MRW_OP
                 * 1600 MHz: 0x54
                 * 1331 MHz: 0x44
                 * 1065 MHz: 0x34
                 *  800 MHz: 0x34
                 *  665 MHz: 0x14
                 *  408 MHz: 0x04
                 *  204 MHz: 0x04
                 *
                 * EMC_MRW2: MRW2_OP
                 * 1600 MHz: 0x2D 45 5*9
                 * 1331 MHz: 0x24 36 4*9
                 * 1065 MHz: 0x1B 27 3*9
                 *  800 MHz: 0x12 18 2*9
                 *  665 MHz: 0x09  9 1*9
                 *  408 MHz: 0x00
                 *  204 MHz: 0x00
                 */
                {

                }

                /* EMC_CFG_2 */
                /* BIT 5:3  - ZQ_EXTRA_DELAY: 6(1600MHz), 5(1331.2MHz), max possible value: 7 */
                {
                    CLEAR_BIT(target_table->emc_cfg_2, 5,3)
                    target_table->emc_cfg_2 |= 7 << 3;
                }
            }
        }
        #endif
    }

    /* Unlock the second sub-partition for retail Mariko, and double the bandwidth (~60GB/s)
     * https://github.com/CTCaer/hekate/blob/01b6e645b3cb69ddf28cc9eff40c4b35bf03dbd4/bdk/mem/sdram.h#L30
     *
     * Sub-partitions are defined as ranks, so there is no other way than replacing DRAM chips.
     */
}

namespace ptm {
    constexpr u32 EmcOffsetStart[] = {
        // 0xC5E24,
        0xA032C,
    };

    constexpr u32 OffsetInterval = 0x20;

    constexpr u32 CpuBoostOffset = 0x170;
}