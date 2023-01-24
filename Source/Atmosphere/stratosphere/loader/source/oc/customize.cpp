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

#include "customize.hpp"

namespace ams::ldr::oc {

volatile CustomizeTable C = {
/* DRAM Timing:
 * AUTO_ADJ_MARIKO_SAFE_NO_ADJ_ERISTA: Auto adjust timings for Mariko LPDDR4X ≤3733 Mbps specs, 8Gb density; No timing adjustment for Erista. (Default)
 * AUTO_ADJ_MARIKO_4266_NO_ADJ_ERISTA: Auto adjust timings for Mariko LPDDR4X 4266 Mbps specs, 8Gb density; No timing adjustments for Erista.
 * NO_ADJ_ALL: No timing adjustment for both Erista and Mariko. Might achieve better performance on Mariko but lower maximum frequency is expected.
 */
.mtcConf = AUTO_ADJ_MARIKO_SAFE_NO_ADJ_ERISTA,

/* Mariko CPU:
 * - Max Clock in kHz:
 *   Default: 1785000
 *   2397000 might be unreachable for some SoCs.
 */
.marikoCpuMaxClock   = 2397000,
/* - Boost Clock in kHz:
 *   Default: 1785000
 *   Boost clock will be applied when applications request higher CPU frequency for quicker loading.
 *   This will be set regardless of whether sys-clk is enabled.
 */
.marikoCpuBoostClock = 1785000,
/* - Max Voltage in mV:
 *   Default voltage: 1120
 */
.marikoCpuMaxVolt    = 1235,

/* Mariko GPU:
 * - Max Clock in kHz:
 *   Default: 921600
 *   NVIDIA Maximum: 1267200
 *   1305600 might be unreachable for some SoCs.
 */
.marikoGpuMaxClock = 1305600,

/* Mariko EMC:
 * - RAM Clock in kHz:
 *   Values should be > 1600000, and divided evenly by 9600.
 *   [WARNING]
 *   RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM:
 *   - Graphical glitches
 *   - System instabilities
 *   - NAND corruption
 */
.marikoEmcMaxClock = 1996800,
/* - RAM Voltage in uV
 *   Range: 600'000 to 650'000 uV
 *   Value should be divided evenly by 5'000
 *   Default: 600'000
 *   Not enabled by default.
 *   This will not work without sys-clk-OC.
 */
.marikoEmcVolt     = 0,

/* Erista CPU:
 * Not tested but enabled by default.
 * - Max Voltage in mV
 */
.eristaCpuMaxVolt  = 1235,

/* Erista EMC:
 * - RAM Clock in kHz
 *   [WARNING]
 *   RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM:
 *   - Graphical glitches
 *   - System instabilities
 *   - NAND corruption
 */
.eristaEmcMaxClock = 1862400,
/* - RAM Voltage in uV
 *   Range: 600'000 to 1250'000 uV
 *   Value should be divided evenly by 12'500
 *   Default(HOS): 1125'000
 *   Not enabled by default.
 */
.eristaEmcVolt     = 0,
};

}