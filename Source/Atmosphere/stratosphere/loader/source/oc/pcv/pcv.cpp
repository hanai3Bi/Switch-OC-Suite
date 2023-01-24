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

namespace ams::ldr::oc::pcv {

Result MemFreqPllmLimit(u32* ptr) {
    clk_pll_param* entry = reinterpret_cast<clk_pll_param *>(ptr);
    R_UNLESS(entry->freq == entry->vco_max, ldr::ResultInvalidMemPllmEntry());

    // Double the max clk simply
    u32 max_clk = entry->freq * 2;
    entry->freq = max_clk;
    entry->vco_max = max_clk;
    R_SUCCEED();
}

void SafetyCheck() {
    if (C.custRev != CUST_REV       ||
        C.marikoCpuMaxVolt >= 1300  ||
        C.eristaCpuMaxVolt >= 1300  ||
        (C.eristaEmcVolt && (C.eristaEmcVolt < 600'000 || C.eristaEmcVolt > 1250'000)) ||
        (C.marikoEmcVolt && (C.marikoEmcVolt < 600'000 || C.marikoEmcVolt > 650'000)))
    {
        CRASH("Triggered");
    }
}

void Patch(uintptr_t mapped_nso, size_t nso_size) {
    #ifdef ATMOSPHERE_IS_STRATOSPHERE
    SafetyCheck();
    bool isMariko = (spl::GetSocType() == spl::SocType_Mariko);
    if (isMariko)
        mariko::Patch(mapped_nso, nso_size);
    else
        erista::Patch(mapped_nso, nso_size);
    #endif
}

}