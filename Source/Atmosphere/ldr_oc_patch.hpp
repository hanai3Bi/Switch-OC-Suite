
constexpr ro::ModuleId PcvModuleId[] = {
    ParseModuleId("91D61D59D7002378E35584FC0B38C7693A3ABAB5"), //11.0.0
    ParseModuleId("C503E96550F302E121873136B814A529863D949B"), //12.x
    ParseModuleId("2058C97C551571506656AA04EC85E2B1B01B155C"), //13.0.0-13.1.0
};

constexpr ro::ModuleId PtmModuleId[] = {
    ParseModuleId("A79706954C6C45568B0FFE610627E2E89D8FB0D4"), //12.x
    ParseModuleId("2CA78D4066C1C11317CC2705EBADA9A51D3AC981"), //13.0.0-13.1.0
};

namespace pcv {
    typedef struct {
        u32 freq = 0;
        u32 unk1 = 0;
        s32 coeffs[6] = {0};
        u32 volt = 0;
        u32 unk3[5] = {0};
    } cpu_freq_cvb_table_t;

    typedef struct {
        u32 freq = 0;
        u32 volt = 0;
        u32 unk[6] = {0};
        s32 coeffs[6] = {0};
    } gpu_cvb_pll_table_t;

    typedef struct {
        u64 freq;
        s32 volt[4] = {0};
    } emc_dvb_dvfs_table_t;

    /* CPU */
    constexpr u32 CpuVoltageLimitOffsets[][11] = {
        { 0xE1A8C, 0xE1A98, 0xE1AA4, 0xE1AB0, 0xE1AF8, 0xE1B04, 0xE1B10, 0xE1B1C, 0xE1B28, 0xE1B34, 0xE1F4C },
        { 0xF08DC, 0xF08E8, 0xF08F4, 0xF0900, 0xF0948, 0xF0954, 0xF0960, 0xF096C, 0xF0978, 0xF0984, 0xF0D9C },
        { 0xF092C, 0xF0938, 0xF0944, 0xF0950, 0xF0998, 0xF09A4, 0xF09B0, 0xF09BC, 0xF09C8, 0xF09D4, 0xF0DEC },
    };
    constexpr u32 NewCpuVoltageLimit = 1220;
    static_assert(NewCpuVoltageLimit <= 1300); //1300mV hangs for me

    constexpr u32 CpuVoltageOldTableCoeff[][10] = {
        { 0xE2140, 0xE2178, 0xE21B0, 0xE21E8, 0xE2220, 0xE2258, 0xE2290, 0xE22C8, 0xE2300, 0xE2338 },
        { 0xF0F90, 0xF0FC8, 0xF1000, 0xF1038, 0xF1070, 0xF10A8, 0xF10E0, 0xF1118, 0xF1150, 0xF1188 },
        { 0xF0FE0, 0xF1018, 0xF1050, 0xF1088, 0xF10C0, 0xF10F8, 0xF1130, 0xF1168, 0xF11A0, 0xF11D8 },
    };
    constexpr u32 NewCpuVoltageCoeff = NewCpuVoltageLimit * 1000;

    constexpr u32 CpuTablesFreeSpace[] = {
        0xE2350,
        0xF11A0,
        0xF11F0,
    };
    // TODO: correctly derive c0-c2 coefficients
    constexpr cpu_freq_cvb_table_t NewCpuTables[] = {
        { 2091000, 0, {1719782, -40440, 27}, NewCpuVoltageCoeff, {} },
        { 2193000, 0, {1809766, -41939, 27}, NewCpuVoltageCoeff, {} },
        { 2295000, 0, {1904458, -43439, 27}, NewCpuVoltageCoeff, {} },
        { 2397000, 0, {2004105, -44938, 27}, NewCpuVoltageCoeff, {} },
    };
    static_assert(sizeof(NewCpuTables) <= sizeof(cpu_freq_cvb_table_t)*14);

    constexpr u32 MaxCpuClockOffset[] = {
        0xE2740,
        0xF1590,
        0xF15E0,
    };
    constexpr u32 NewMaxCpuClock = 2397000;

    /* GPU */
    constexpr u32 GpuTablesFreeSpace[] = {
        0xE3410,
        0xF2260,
        0xF22B0,
    };
    // TODO: correctly derive c0-c5 coefficients
    constexpr gpu_cvb_pll_table_t NewGpuTables[] = {
        { 1305600, 0, {}, {1380113, -13465, -874, 0, 2580, 648} },
        { 1344000, 0, {}, {1420000, -14000, -870, 0, 2193, 824} },
    };
    static_assert(sizeof(NewGpuTables) <= sizeof(gpu_cvb_pll_table_t)*15);

    constexpr u32 Reg1MaxGpuOffset[] = {
        0x2E0AC,
        0x3F6CC,
        0x3F12C,
    };
    constexpr u8 Reg1NewMaxGpuClock[][0xC] = {
        /* Original: 1267MHz
         *
         * MOV   W13,#0x5600
         * MOVK  W13,#0x13,LSL #16
         * NOP
         *
         * Bump to 1536MHz
         *
         * MOV   W13,#0x7000
         * MOVK  W13,#0x17,LSL #16
         * NOP
         */
        { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
    };

    constexpr u32 Reg2MaxGpuOffset[] = {
        0x2E110,
        0x3F730,
        0x3F190,
    };
    constexpr u8 Reg2NewMaxGpuClock[][0x8] = {
        /* Original: 1267MHz
         *
         * MOV   W13,#0x5600
         * MOVK  W13,#0x13,LSL #16
         *
         * Bump to 1536MHz
         *
         * MOV   W13,#0x7000
         * MOVK  W13,#0x17,LSL #16
         */
        { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, },
        { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, },
    };

    /* EMC */
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
    // 0x1428B8    1125000         #0x20 // min voltage, default voltage for Erista EMC
    // 0x1428BC    1125000         #0x24 // max voltage, default voltage for Erista EMC
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
    // What about DRAM on PHASE31?

    // max77812 document: https://datasheets.maximintegrated.com/en/ds/MAX77812.pdf

    // See if we can read/query max77812 pmic via i2c for voltage info in fusee/hekate

    // TODO: investigate why frequencies lower than 1331 MHz cannot be set
    constexpr u32 EmcFreqOffsets[][30] = {
        { 0xD7C60, 0xD7C68, 0xD7C70, 0xD7C78, 0xD7C80, 0xD7C88, 0xD7C90, 0xD7C98, 0xD7CA0, 0xD7CA8, 0xE1800, 0xEEFA0, 0xF2478, 0xFE284, 0x10A304, 0x10D7DC, 0x110A40, 0x113CA4, 0x116F08, 0x11A16C, 0x11D3D0, 0x120634, 0x123898, 0x126AFC, 0x129D60, 0x12CFC4, 0x130228, 0x13BFE0, 0x140D00, 0x140D50, },
        { 0xE1810, 0xE6530, 0xE6580, 0xE6AB0, 0xE6AB8, 0xE6AC0, 0xE6AC8, 0xE6AD0, 0xE6AD8, 0xE6AE0, 0xE6AE8, 0xE6AF0, 0xE6AF8, 0xF0650, 0xFDDF0, 0x1012C8, 0x10D0D4, 0x119154, 0x11C62C, 0x11F890, 0x122AF4, 0x125D58, 0x128FBC, 0x12C220, 0x12F484, 0x1326E8, 0x13594C, 0x138BB0, 0x13BE14, 0x13F078, },
        { 0xE1860, 0xE6580, 0xE65D0, 0xE6B00, 0xE6B08, 0xE6B10, 0xE6B18, 0xE6B20, 0xE6B28, 0xE6B30, 0xE6B38, 0xE6B40, 0xE6B48, 0xF06A0, 0xFDE40, 0x101318, 0x10D124, 0x1191A4, 0x11C67C, 0x11F8E0, 0x122B44, 0x125DA8, 0x12900C, 0x12C270, 0x12F4D4, 0x132738, 0x13599C, 0x138C00, 0x13BE64, 0x13F0C8, },
    };
}

namespace ptm {
    constexpr u32 EmcOffsetStart[] = {
        0xC5E24,
        0xA032C,
    };

    constexpr u32 OffsetInterval = 0x20;

    constexpr u32 CpuBoostOffset = 0x170;
}