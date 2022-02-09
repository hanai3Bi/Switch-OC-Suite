namespace ams::ldr::oc {
#include "mtc_empty_table.inc"
static const volatile CustomizeTable C = {
/* DRAM Timing:
 * AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density (Default).
 * AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.
 * ENTIRE_TABLE_ERISTA/ENTIRE_TABLE_MARIKO:
 *                       Replace the entire max mtc table with customized one (provided by user).
 */
.mtcConf = AUTO_ADJ_MARIKO_SAFE,

/* Mariko CPU:
 * - Max Clock in kHz:
 *   Default: 1785000
 *   >= 2193000 will enable overvolting (> 1120 mV)
 * - Max Voltage in mV:
 *   Default voltage: 1120
 *   Haven't tested anything higher than 1220.
 */
.marikoCpuMaxClock = 2397000,
.marikoCpuMaxVolt  = 1220,

/* Mariko GPU:
 * - Max Clock in kHz:
 *   Default: 921600
 *   NVIDIA Maximum: 1267200
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
 *   Timings from auto-adjustment have been tested safe for up to 1996.8 MHz for all DRAM chips.
 */
.marikoEmcMaxClock = 1996800,

/* Erista CPU:
 * Untested and not enabled by default.
 * - Enable Overclock
 *   Require modificaitions towards NewCpuTables!
 * - Max Voltage in mV
 */
.eristaCpuOCEnable = 0,
.eristaCpuMaxVolt  = 0,

/* Erista EMC:
 * - RAM Clock in kHz
 *   [WARNING]
 *   RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM:
 *   - Graphical glitches
 *   - System instabilities
 *   - NAND corruption
 * - RAM Voltage in uV
 *   Range: 600'000 to 1250'000 uV
 *   Value should be divided evenly by 12'500
 *   Default(HOS): 1125'000
 *   Not enabled by default.
 */
.eristaEmcMaxClock = 1862400,
.eristaEmcVolt     = 0,

.eristaMtc = reinterpret_cast<EristaMtcTable *>(const_cast<u8 *>(EmptyMtcTable)),
};
}