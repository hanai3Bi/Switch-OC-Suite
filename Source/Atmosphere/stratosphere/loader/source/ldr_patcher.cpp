/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "ldr_patcher.hpp"
//#define ADJUST_TIMING

namespace ams::ldr {

    namespace {

        constexpr const char *NsoPatchesDirectory = "exefs_patches";

        /* Exefs patches want to prevent modification of header, */
        /* and also want to adjust offset relative to mapped location. */
        constexpr size_t NsoPatchesProtectedSize   = sizeof(NsoHeader);
        constexpr size_t NsoPatchesProtectedOffset = sizeof(NsoHeader);

        constexpr const char * const LoaderSdMountName = "#amsldr-sdpatch";
        static_assert(sizeof(LoaderSdMountName) <= fs::MountNameLengthMax);

        constinit os::SdkMutex g_ldr_sd_lock;
        constinit bool g_mounted_sd;

        constinit os::SdkMutex g_embedded_patch_lock;
        constinit bool g_got_embedded_patch_settings;
        constinit bool g_force_enable_usb30;

        bool EnsureSdCardMounted() {
            std::scoped_lock lk(g_ldr_sd_lock);

            if (g_mounted_sd) {
                return true;
            }

            if (!cfg::IsSdCardInitialized()) {
                return false;
            }

            if (R_FAILED(fs::MountSdCard(LoaderSdMountName))) {
                return false;
            }

            return (g_mounted_sd = true);
        }

        bool IsUsb30ForceEnabled() {
            std::scoped_lock lk(g_embedded_patch_lock);

            if (!g_got_embedded_patch_settings) {
                g_force_enable_usb30 = spl::IsUsb30ForceEnabled();
                g_got_embedded_patch_settings = true;
            }

            return g_force_enable_usb30;
        }

        u32 GetEmcClock() {
            // RAM freqs to choose: 1600000, 1728000, 1795200, 1862400, 1894400, 1932800, 1996800, 2064000, 2099200, 2131200
            // RAM overclock could be UNSTABLE on some RAM without bumping up voltage,
            // and therefore show graphical glitches, hang randomly or even worse, corrupt your NAND
            return 1862400;
        }

        u32 GetCpuBoostClock() {
            return 1963500;
        }

        consteval u8 ParseNybble(char c) {
            AMS_ASSUME(('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'));
            if ('0' <= c && c <= '9') {
                return c - '0' + 0x0;
            } else if ('A' <= c && c <= 'F') {
                return c - 'A' + 0xA;
            } else /* if ('a' <= c && c <= 'f') */ {
                return c - 'a' + 0xa;
            }
        }

        consteval ro::ModuleId ParseModuleId(const char *str) {
            /* Parse a static module id. */
            ro::ModuleId module_id = {};

            size_t ofs = 0;
            while (str[0] != 0) {
                AMS_ASSUME(ofs < sizeof(module_id));
                AMS_ASSUME(str[1] != 0);

                module_id.data[ofs] = (ParseNybble(str[0]) << 4) | (ParseNybble(str[1]) << 0);

                str += 2;
                ofs++;
            }

            return module_id;
        }

        struct EmbeddedPatchEntry {
            uintptr_t offset;
            const void * const data;
            size_t size;
        };

        struct EmbeddedPatch {
            ro::ModuleId module_id;
            size_t num_entries;
            const EmbeddedPatchEntry *entries;
        };

        #include "ldr_embedded_usb_patches.inc"

    }

    #include "ldr_oc_patch.hpp"

    /* Apply IPS patches. */
    void LocateAndApplyIpsPatchesToModule(const u8 *module_id_data, uintptr_t mapped_nso, size_t mapped_size) {
        if (!EnsureSdCardMounted()) {
            return;
        }

        ro::ModuleId module_id;
        std::memcpy(std::addressof(module_id.data), module_id_data, sizeof(module_id.data));
        ams::patcher::LocateAndApplyIpsPatchesToModule(LoaderSdMountName, NsoPatchesDirectory, NsoPatchesProtectedSize, NsoPatchesProtectedOffset, std::addressof(module_id), reinterpret_cast<u8 *>(mapped_nso), mapped_size);
    }

    /* Apply embedded patches. */
    void ApplyEmbeddedPatchesToModule(const u8 *module_id_data, uintptr_t mapped_nso, size_t mapped_size) {
        /* Make module id. */
        ro::ModuleId module_id;
        std::memcpy(std::addressof(module_id.data), module_id_data, sizeof(module_id.data));

        if (IsUsb30ForceEnabled()) {
            for (const auto &patch : Usb30ForceEnablePatches) {
                if (std::memcmp(std::addressof(patch.module_id), std::addressof(module_id), sizeof(module_id)) == 0) {
                    for (size_t i = 0; i < patch.num_entries; ++i) {
                        const auto &entry = patch.entries[i];
                        if (entry.offset + entry.size <= mapped_size) {
                            std::memcpy(reinterpret_cast<void *>(mapped_nso + entry.offset), entry.data, entry.size);
                        }
                    }
                }
            }
        }

        u32 EmcClock = GetEmcClock();
        if (spl::GetSocType() == spl::SocType_Mariko && EmcClock) {
            for (u32 i = 0; i < sizeof(PcvModuleId)/sizeof(ro::ModuleId); i++) {
                if (std::memcmp(std::addressof(PcvModuleId[i]), std::addressof(module_id), sizeof(module_id)) == 0) {
                    /* Add new CPU and GPU clock tables for Mariko */
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::CpuTablesFreeSpace[i]), pcv::NewCpuTables, sizeof(pcv::NewCpuTables));
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::GpuTablesFreeSpace[i]), pcv::NewGpuTables, sizeof(pcv::NewGpuTables));

                    /* Patch Mariko max CPU and GPU clockrates */
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::MaxCpuClockOffset[i]), &pcv::NewMaxCpuClock, sizeof(pcv::NewMaxCpuClock));
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::Reg1MaxGpuOffset[i]), pcv::Reg1NewMaxGpuClock, sizeof(pcv::Reg1NewMaxGpuClock[i]));
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::Reg2MaxGpuOffset[i]), pcv::Reg2NewMaxGpuClock, sizeof(pcv::Reg2NewMaxGpuClock[i]));

                    /* Patch max cpu voltage on Mariko */
                    for (u32 j = 0; j < sizeof(pcv::CpuVoltageLimitOffsets[i])/sizeof(u32); j++) {
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::CpuVoltageLimitOffsets[i][j]), &pcv::NewCpuVoltageLimit, sizeof(pcv::NewCpuVoltageLimit));
                    }
                    for (u32 j = 0; j < sizeof(pcv::CpuVoltageOldTableCoeff[i])/sizeof(u32); j++) {
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::CpuVoltageOldTableCoeff[i][j]), &pcv::NewCpuVoltageScaled, sizeof(pcv::NewCpuVoltageScaled));
                    }

                    /* Patch max GPU voltage on Mariko */
                    std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::GpuVoltageLimitOffsets[i]), &pcv::NewGpuVoltageLimit, sizeof(pcv::NewGpuVoltageLimit));

                    /* Patch RAM Clock */
                    for (u32 j = 0; j < sizeof(pcv::EmcFreqOffsets[i])/sizeof(u32); j++) {
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + pcv::EmcFreqOffsets[i][j]), &EmcClock, sizeof(EmcClock));
                    }

                #ifdef ADJUST_TIMING
                    /* Adjust timing parameters in 1600MHz mtc tables */
                    u32 param_1331, param_1600;
                    #define ADJUST_PROPORTIONAL(TARGET_TABLE, REF_TABLE, PARAM)                                                \
                        param_1331 = REF_TABLE->PARAM;                                                                         \
                        param_1600 = TARGET_TABLE->PARAM;                                                                      \
                        TARGET_TABLE->PARAM = param_1331 + ((GetEmcClock()-1331200)*(param_1600-param_1331))/(1600000-1331200);

                    #define ADJUST_PROP_WITHIN_ALL_REG(TARGET_TABLE, REF_TABLE, PARAM)            \
                        ADJUST_PROPORTIONAL(TARGET_TABLE, REF_TABLE, burst_regs.PARAM)            \
                        ADJUST_PROPORTIONAL(TARGET_TABLE, REF_TABLE, shadow_regs_ca_train.PARAM)  \
                        ADJUST_PROPORTIONAL(TARGET_TABLE, REF_TABLE, shadow_regs_rdwr_train.PARAM)

                    /* Calculate DIVM and DIVN */
                    /* Assume oscillator (PLLMB_IN) is 38.4 MHz */
                    /* PLLMB_OUT = PLLMB_IN / DIVM * DIVN */
                    u32 divn = GetEmcClock() / 38400;
                    u32 divm = 1;
                    if (GetEmcClock() - divn * 38400 >= 38400 / 2) {
                        divm = 2;
                        divn = divn * 2 + 1;
                    }

                    if (i == 2) {
                        for (u32 j = 0; j < sizeof(pcv::MtcTable_1600)/sizeof(u32); j++) {
                            pcv::MarikoMtcTable* mtc_table_1600 = reinterpret_cast<pcv::MarikoMtcTable *>(mapped_nso + pcv::MtcTable_1600[j]);
                            pcv::MarikoMtcTable* mtc_table_1331 = reinterpret_cast<pcv::MarikoMtcTable *>(mapped_nso + pcv::MtcTable_1600[j] - pcv::MtcTableOffset);

                            /* Normal and reasonable values */
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rc);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rfc);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rfcpb);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_ras);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rp);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_w2r);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_r2p);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_w2p);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_trtm);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_twtm);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tratm);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_twatm);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rd_rcd);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_wr_rcd);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rrd);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_wdv);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_wsv);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_wev);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_wdv_mask); //?
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_quse);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_quse_width);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_einput);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_einput_duration);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_qsafe);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rdv);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rdv_mask); //?
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rdv_early);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rdv_early_mask); //?
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_refresh);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pre_refresh_req_cnt);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pdex2wr);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pdex2rd);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_act2pden);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_rw2pden);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_cke2pden);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pdex2mrr);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_txsr);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_txsrdll);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tcke);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tckesr);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tpd);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tfaw);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_trpab);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tclkstop);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_trefbw);
                          //ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pmacro_ddll_long_cmd_4); //?
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_pmacro_dll_cfg_2);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_qpop);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tr_rdv);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tr_qpop);
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tr_rdv_mask); //?
                            ADJUST_PROP_WITHIN_ALL_REG(mtc_table_1600, mtc_table_1331, emc_tr_qsafe);

                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, dram_timings.rl);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_rcd);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_rp);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_rc);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_ras);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_faw);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_wap2pre);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_r2w);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_w2r);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, burst_mc_regs.mc_emem_arb_timing_rfcpb);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, la_scale_regs.mc_mll_mpcorer_ptsa_rate);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, min_mrs_wait);
                            ADJUST_PROPORTIONAL(mtc_table_1600, mtc_table_1331, latency);

                            mtc_table_1600->pllmb_divm = divm;
                            mtc_table_1600->pllmb_divn = divn;
                        }
                    }
                #endif
                }
            }

            EmcClock = GetEmcClock() * 1000;

            u32 CpuBoostClock = GetCpuBoostClock() * 1000;

            for (u32 i = 0; i < sizeof(PtmModuleId)/sizeof(ro::ModuleId); i++) {
                if (std::memcmp(std::addressof(PtmModuleId[i]), std::addressof(module_id), sizeof(module_id)) == 0) {
                    for (u32 j = 0; j < 16; j++) {
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + ptm::EmcOffsetStart[i] + ptm::OffsetInterval * j), &EmcClock, sizeof(EmcClock));
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + ptm::EmcOffsetStart[i] + ptm::OffsetInterval * j + 0x4), &EmcClock, sizeof(EmcClock));
                    }
                    for (u32 j = 0; j < 2; j++) {
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + ptm::EmcOffsetStart[i] + ptm::CpuBoostOffset + ptm::OffsetInterval * j), &CpuBoostClock, sizeof(CpuBoostClock));
                        std::memcpy(reinterpret_cast<void *>(mapped_nso + ptm::EmcOffsetStart[i] + ptm::CpuBoostOffset + ptm::OffsetInterval * j + 0x4), &CpuBoostClock, sizeof(CpuBoostClock));
                    }
                }
            }
        }
    }

}