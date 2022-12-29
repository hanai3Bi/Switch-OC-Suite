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

#include "pcv_erista.hpp"

namespace ams::ldr::oc::pcv::erista {

Result CpuFreqCvbTable(u32* ptr) {
    cpu_freq_cvb_table_t* default_end = reinterpret_cast<cpu_freq_cvb_table_t *>(ptr);
    cpu_freq_cvb_table_t* new_start = default_end + 1;

    // Validate existing table
    void* cpu_cvb_table_head = reinterpret_cast<u8 *>(new_start) - sizeof(CpuCvbTableDefault);
    bool validated = std::memcmp(cpu_cvb_table_head, CpuCvbTableDefault, sizeof(CpuCvbTableDefault)) == 0;
    R_UNLESS(validated, ldr::ResultInvalidCpuDvfs());

    if (!C.eristaCpuOCEnabled)
        R_SKIP();

    std::memcpy(reinterpret_cast<void *>(new_start), CpuCvbTableAppend, sizeof(CpuCvbTableAppend));

    R_SUCCEED();
}

Result CpuVoltRange(u32* ptr) {
    u32 min_volt_got = *(ptr - 1);
    for (const auto& mv : CpuMinVolts) {
        if (min_volt_got != mv)
            continue;

        if (!C.eristaCpuMaxVolt)
            R_SKIP();

        PatchOffset(ptr, C.eristaCpuMaxVolt);
        R_SUCCEED();
    }
    R_THROW(ldr::ResultInvalidCpuMinVolt());
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

    if (C.eristaEmcMaxClock <= MemClkOSLimit)
        R_SKIP();

    // Make room for new mtc table, discarding useless 40.8 MHz table
    // 40800 overwritten by 68000, ..., 1331200 overwritten by 1600000, leaving table_list[0] not overwritten
    for (u32 i = khz_list_size - 1; i > 0; i--)
        std::memcpy(static_cast<void *>(table_list[i]), static_cast<void *>(table_list[i - 1]), sizeof(EristaMtcTable));

    PatchOffset(ptr, C.eristaEmcMaxClock);

    R_SUCCEED();
}

Result MemFreqMax(u32* ptr) {
    if (C.eristaEmcMaxClock <= MemClkOSLimit)
        R_SKIP();

    PatchOffset(ptr, C.eristaEmcMaxClock);

    R_SUCCEED();
}

Result MemVoltHandler(u32* ptr) {
    u32 emc_uv = C.eristaEmcVolt;
    if (!emc_uv)
        R_SKIP();

    constexpr u32 uv_step = 12'500;
    if (emc_uv % uv_step)
        emc_uv = emc_uv / uv_step * uv_step; // rounding

    PatchOffset(ptr, emc_uv);

    R_SUCCEED();
}

void Patch(uintptr_t mapped_nso, size_t nso_size) {
    PatcherEntry<u32> patches[] = {
        { "CPU Freq Table", &CpuFreqCvbTable,   1, nullptr, CpuClkOSLimit },
        { "CPU Volt Limit", &CpuVoltRange,      0, &CpuMaxVoltPatternFn },
        { "MEM Freq Mtc",   &MemFreqMtcTable,   0, nullptr, MemClkOSLimit },
        { "MEM Freq Max",   &MemFreqMax,        0, nullptr, MemClkOSLimit },
        { "MEM Freq PLLM",  &MemFreqPllmLimit,  2, nullptr, MemClkPllmLimit },
        { "MEM Volt",       &MemVoltHandler,    2, nullptr, MemVoltHOS },
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