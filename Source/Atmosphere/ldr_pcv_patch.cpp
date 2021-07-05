// placed in Atmosphere/stratosphere/loader/source/

#include <stratosphere.hpp>

#define EMC_OVERCLOCK 1
#define EMC_OVERVOLT 1

namespace ams::ldr {

    namespace {

        constexpr u32 CtestOffset[2] = { 0xF400, 0x1079C };

        constexpr u32 CopyrightOffset[2] = { 0xC6128, 0xCA414 }; //am_no_copyright port

        constexpr u8 BehemothPatch[8] = { 0xE0, 0x03, 0x1F, 0xAA, 0xC0, 0x03, 0x5F, 0xD6 }; //🤔

        typedef struct {
            u32 hz = 0;
            u32 volt = 0;
            u32 unk[6] = {0};
            s32 coeffs[6] = {0};
        } gpu_clock_table_t;

        constexpr u32 EmcFreqOffsets[2][30] = {
            { 0xD7C60, 0xD7C68, 0xD7C70, 0xD7C78, 0xD7C80, 0xD7C88, 0xD7C90, 0xD7C98, 0xD7CA0, 0xD7CA8, 0xE1800, 0xEEFA0, 0xF2478, 0xFE284, 0x10A304, 0x10D7DC, 0x110A40, 0x113CA4, 0x116F08, 0x11A16C, 0x11D3D0, 0x120634, 0x123898, 0x126AFC, 0x129D60, 0x12CFC4, 0x130228, 0x13BFE0, 0x140D00, 0x140D50, },
            { 0xE1810, 0xE6530, 0xE6580, 0xE6AB0, 0xE6AB8, 0xE6AC0, 0xE6AC8, 0xE6AD0, 0xE6AD8, 0xE6AE0, 0xE6AE8, 0xE6AF0, 0xE6AF8, 0xF0650, 0xFDDF0, 0x1012C8, 0x10D0D4, 0x119154, 0x11C62C, 0x11F890, 0x122AF4, 0x125D58, 0x128FBC, 0x12C220, 0x12F484, 0x1326E8, 0x13594C, 0x138BB0, 0x13BE14, 0x13F078, }
        };
        // RAM freqs to choose: 1731200, 1862400, 1996800, 2131200
        constexpr u32 NewEmcFreq = 1862400;
        // RAM overclock could be UNSTABLE on some RAM without bumping up voltage,
        // and therefore show graphical glitches, hang randomly or even worse, corrupt your NAND

        namespace Erista {

            typedef struct {
                u32 hz = 0;
                u32 unk = 0;
                u32 volt = 0;
                u32 unk2[5] = {0};
                s32 coeffs[6] = {0};
            } cpu_clock_table_t;

            /* CPU */
            constexpr u32 CpuVoltageLimitOffsets[2][3] = {
                { 0xE1AC8, 0xE1AD4, 0xE37E4 },
                { 0xF0918, 0xF0924, 0xF2634 },
            };
            constexpr u32 NewCpuVoltageLimit = 1358;
            static_assert(NewCpuVoltageLimit <= 1400);

            constexpr u32 CpuTablesFreeSpace[2] = { 0xE3B78, 0xF29C8 };
            constexpr cpu_clock_table_t NewCpuTables[] = {
                { 1887000, 0, 1240000, {}, { 5100873, -279186, 4747 } },
                { 1963500, 0, 1262000, {}, { 5100873, -279186, 4747 } },
                { 2091000, 0, 1298000, {}, { 5100873, -279186, 4747 } },
                { 2193000, 0, 1328000, {}, { 5100873, -279186, 4747 } },
                { 2295000, 0, 1358000, {}, { 5100873, -279186, 4747 } },
            };
            static_assert(sizeof(NewCpuTables) <= sizeof(cpu_clock_table_t)*16);

            /* GPU */
            constexpr u32 GpuTablesFreeSpace[2] = { 0xE2B58, 0xF19A8 };
            constexpr gpu_clock_table_t NewGpuTables[] = {
                {  998400, 0, {}, { 1316991, 8144, -940, 808, -21583, 226 } },
                { 1075200, 0, {}, { 1358883, 8144, -940, 808, -21583, 226 } },
            };
            static_assert(sizeof(NewGpuTables) <= sizeof(gpu_clock_table_t)*20);

            /* EMC */
            constexpr u32 EmcVolatageOffsets[2][2] = {
                { 0x143998, 0x14399C },
                { 0x142878, 0x14287C },
            };
            constexpr u32 NewEmcVoltage = 1150000;
            static_assert(NewEmcVoltage <= 1250000);
            // 1150mV for 1862.4 MHz and 1200 mV for 2131.2 MHz, according to the feedback in RetroNX Discord
            // 1125mV(HOS default) for 1731.2 MHz and 1175mV for 1996.8 MHz
        };

        namespace Mariko {

            typedef struct {
                u32 hz = 0;
                u32 unk1 = 0;
                s32 coeffs[6] = {0};
                u32 volt = 0;
                u32 unk3[5] = {0};
            } cpu_clock_table_t;

            /* CPU */
            constexpr u32 CpuVoltageLimitOffsets[2][11] = { 
                { 0xE1A8C, 0xE1A98, 0xE1AA4, 0xE1AB0, 0xE1AF8, 0xE1B04, 0xE1B10, 0xE1B1C, 0xE1B28, 0xE1B34, 0xE1F4C },
                { 0xF08DC, 0xF08E8, 0xF08F4, 0xF0900, 0xF0948, 0xF0954, 0xF0960, 0xF096C, 0xF0978, 0xF0984, 0xF0D9C },
            };
            constexpr u32 NewCpuVoltageLimit = 1220;
            static_assert(NewCpuVoltageLimit <= 1300); //1300mV hangs for me

            constexpr u32 CpuVoltageCoeffTable[2][10] = { 
                { 0xE2140, 0xE2178, 0xE21B0, 0xE21E8, 0xE2220, 0xE2258, 0xE2290, 0xE22C8, 0xE2300, 0xE2338 },
                { 0xF0F90, 0xF0FC8, 0xF1000, 0xF1038, 0xF1070, 0xF10A8, 0xF10E0, 0xF1118, 0xF1150, 0xF1188 },
            };
            constexpr u32 NewCpuVoltageCoeff = NewCpuVoltageLimit * 1000;

            constexpr u32 CpuTablesFreeSpace[2] = { 0xE2350, 0xF11A0 };
            constexpr cpu_clock_table_t NewCpuTables[] = {
                {2091000, 0, {1719782, -40440, 27}, NewCpuVoltageCoeff, {}},
                {2193000, 0, {1809766, -41939, 27}, NewCpuVoltageCoeff, {}},
                {2295000, 0, {1904458, -43439, 27}, NewCpuVoltageCoeff, {}},
                {2397000, 0, {2004105, -44938, 27}, NewCpuVoltageCoeff, {}},
              /*{2499000, 0, {2108966, -46437, 27}, NewCpuVoltageCoeff, {}},
                {2601000, 0, {2219313, -47937, 27}, NewCpuVoltageCoeff, {}},
                {2703000, 0, {2335434, -49436, 27}, NewCpuVoltageCoeff, {}},
                {2805000, 0, {2457630, -50936, 27}, NewCpuVoltageCoeff, {}},
                {2907000, 0, {2586221, -52435, 27}, NewCpuVoltageCoeff, {}},
                {3009000, 0, {2721539, -53934, 27}, NewCpuVoltageCoeff, {}},*/
                // calculated using linear regression:
                //   coeffs[0]=604531*exp(0.0000005*hz)
                //   coeffs[1]=-0.0147*hz-9702.1
            };
            static_assert(sizeof(NewCpuTables) <= sizeof(cpu_clock_table_t)*14);

            constexpr u32 MaxCpuClockOffset[2] = { 0xE2740, 0xF1590 };
            constexpr u32 NewMaxCpuClock = 2397000;

            /* GPU */
            constexpr u32 GpuTablesFreeSpace[2] = { 0xE3410, 0xF2260 };
            constexpr gpu_clock_table_t NewGpuTables[] = {
                { 1305600, 0, {}, {1380113, -13465, -874, 0, 2580, 648} },
                { 1344000, 0, {}, {1420000, -14000, -870, 0, 2193, 824} },
              /*{ 1382400, 0, {}, {1423200, -14188, -868, 0, 2150, 830} },
                { 1420800, 0, {}, {1426290, -14368, -860, 0, 2100, 850} },
                { 1305600, 0, {}, {1380113, -13465, -874, 0, 2580, 648} },
                { 1344000, 0, {}, {1420000, -14100, -870, 0, 2193, 824} },
                { 1382400, 0, {}, {1426290, -14368, -860, 0, 2100, 850} },
                { 1420800, 0, {}, {1450000, -14600, -860, 0, 2000, 850} },
                { 1382400, 0, {}, {1454061, -14500, -868, 0, 2000, 900} },
                { 1382400, 0, {}, {1474061, -15277, -866, 0, 1710, 1024} },
                { 1382400, 0, {}, {1500000, -15500, -880, 0, 1000, 548} },
                { 1420800, 0, {}, {1550000, -16500, -885, 0, 1500, 300} },*/
                // some arbitrary coeffs I guessed and tested, YMMV
            };
            static_assert(sizeof(NewGpuTables) <= sizeof(gpu_clock_table_t)*15);

            constexpr u32 Reg1MaxGpuOffset[2] = { 0x2E0AC, 0x3F6CC };
            constexpr u8 Reg1NewMaxGpuClock[2][0xC] = {
                // Original: 1267MHz
                /*
                MOV   W13,#0x5600
                MOVK  W13,#0x13,LSL #16
                NOP
                */
                // Bump to 1536MHz
                /*
                MOV   W13,#0x7000
                MOVK  W13,#0x17,LSL #16
                NOP
                */
                //0x0D, 0xC0, 0x8A, 0x52, 0x6D, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5
                { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
                { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, 0x1F, 0x20, 0x03, 0xD5 },
            };

            constexpr u32 Reg2MaxGpuOffset[2] = { 0x2E110, 0x3F730 };
            constexpr u8 Reg2NewMaxGpuClock[2][0x8] = {
                // Original: 1267MHz
                /*
                MOV   W13,#0x5600
                MOVK  W13,#0x13,LSL #16
                */
                // Bump to 1536MHz
                /*
                MOV   W13,#0x7000
                MOVK  W13,#0x17,LSL #16
                */
                //0x0D, 0xC0, 0x8A, 0x52, 0x6D, 0x02, 0xA0, 0x72
                { 0x0D, 0x00, 0x8E, 0x52, 0xED, 0x02, 0xA0, 0x72, },
                { 0x0B, 0x00, 0x8E, 0x52, 0xEB, 0x02, 0xA0, 0x72, },
            };

            /* EMC */
            // Not available on Mariko
            /*constexpr u32 EmcVolatageOffsets[2][2] = {
                {
                    0x145FE4, //max77812_dram
                    0x144BA4, //max77812_dram
                },
                {
                    0x143A84,
                    0x144EC4,
                }
            };
            constexpr u32 NewEmcVoltage = 650000;
            static_assert(NewEmcVoltage <= 750000);*/
        };

    }

    void ApplyPcvPatch(u8 *mapped_module, size_t mapped_size, int i) {

        /* Add new CPU and GPU clock tables for Erista */
        AMS_ABORT_UNLESS(Erista::CpuTablesFreeSpace[i] <= mapped_size && Erista::GpuTablesFreeSpace[i] <= mapped_size);
        std::memcpy(mapped_module + Erista::CpuTablesFreeSpace[i], Erista::NewCpuTables, sizeof(Erista::NewCpuTables));
        std::memcpy(mapped_module + Erista::GpuTablesFreeSpace[i], Erista::NewGpuTables, sizeof(Erista::NewGpuTables));

        /* Patch max CPU voltage on Erista */
        for(int j = 0; j < 3; j++) {
            std::memcpy(mapped_module + Erista::CpuVoltageLimitOffsets[i][j], &Erista::NewCpuVoltageLimit, sizeof(Erista::NewCpuVoltageLimit));
        }

        /* Add new CPU and GPU clock tables for Mariko */
        AMS_ABORT_UNLESS(Mariko::CpuTablesFreeSpace[i] <= mapped_size && Mariko::GpuTablesFreeSpace[i] <= mapped_size);
        std::memcpy(mapped_module + Mariko::CpuTablesFreeSpace[i], Mariko::NewCpuTables, sizeof(Mariko::NewCpuTables));
        std::memcpy(mapped_module + Mariko::GpuTablesFreeSpace[i], Mariko::NewGpuTables, sizeof(Mariko::NewGpuTables));

        /* Patch Mariko max CPU and GPU clockrates */
        AMS_ABORT_UNLESS(Mariko::MaxCpuClockOffset[i] <= mapped_size && Mariko::Reg1MaxGpuOffset[i] <= mapped_size && Mariko::Reg2MaxGpuOffset[i] <= mapped_size);
        std::memcpy(mapped_module + Mariko::MaxCpuClockOffset[i], &Mariko::NewMaxCpuClock, sizeof(Mariko::NewMaxCpuClock));
        std::memcpy(mapped_module + Mariko::Reg1MaxGpuOffset[i], Mariko::Reg1NewMaxGpuClock, sizeof(Mariko::Reg1NewMaxGpuClock[i]));
        std::memcpy(mapped_module + Mariko::Reg2MaxGpuOffset[i], Mariko::Reg2NewMaxGpuClock, sizeof(Mariko::Reg2NewMaxGpuClock[i]));

        /* Patch max cpu voltage on Mariko */
        for(int j = 0; j < 11; j++) {
            std::memcpy(mapped_module + Mariko::CpuVoltageLimitOffsets[i][j], &Mariko::NewCpuVoltageLimit, sizeof(Mariko::NewCpuVoltageLimit));
        }
        for(int j = 0; j < 10; j++) {
            std::memcpy(mapped_module + Mariko::CpuVoltageCoeffTable[i][j], &Mariko::NewCpuVoltageCoeff, sizeof(Mariko::NewCpuVoltageCoeff));
        }

        /* Patch EMC clocks and voltage if enabled.
           Note: On Erista, this requires removing or modifiying minerva */
        if constexpr(EMC_OVERCLOCK) {
            for(u32 j = 0; j < sizeof(EmcFreqOffsets[i])/sizeof(u32); j++) {
                AMS_ABORT_UNLESS(EmcFreqOffsets[i][j] <= mapped_size);
                std::memcpy(mapped_module + EmcFreqOffsets[i][j], &NewEmcFreq, sizeof(NewEmcFreq));
            }
        }

        if constexpr(EMC_OVERVOLT) {
            if(spl::GetSocType() == spl::SocType_Erista) {
                for(u32 j = 0; j < sizeof(Erista::EmcVolatageOffsets[i])/sizeof(u32); j++) {
                    AMS_ABORT_UNLESS(Erista::EmcVolatageOffsets[i][j] <= mapped_size);
                    std::memcpy(mapped_module + Erista::EmcVolatageOffsets[i][j], &Erista::NewEmcVoltage, sizeof(Erista::NewEmcVoltage));
                }
            }
            // Not available on Mariko
            /*else if(spl::GetSocType() == spl::SocType_Mariko) {
                for(u32 j = 0; j < sizeof(Mariko::EmcVolatageOffsets[i])/sizeof(u32); j++) {
                    AMS_ABORT_UNLESS(Mariko::EmcVolatageOffsets[i][j] <= mapped_size);
                    std::memcpy(mapped_module + Mariko::EmcVolatageOffsets[i][j], &Mariko::NewEmcVoltage, sizeof(Mariko::NewEmcVoltage));
                }
            }*/
        }

        return;
    }

    void ApplyCtestPatch(u8 *mapped_module, size_t mapped_size, int i) {
        AMS_ABORT_UNLESS(CtestOffset[i] - 0x100 <= mapped_size);
        std::memcpy(mapped_module + CtestOffset[i] - 0x100, BehemothPatch, sizeof(BehemothPatch));
    }

    void ApplyCopyrightPatch(u8 *mapped_module, size_t mapped_size, int i) {
        AMS_ABORT_UNLESS(CopyrightOffset[i] - 0x100 <= mapped_size);
        std::memcpy(mapped_module + CopyrightOffset[i] - 0x100, BehemothPatch, sizeof(BehemothPatch));
    }

}
