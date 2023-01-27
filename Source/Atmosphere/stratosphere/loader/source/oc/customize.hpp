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

#pragma once

#define CUST_REV 3

#include "oc_common.hpp"

namespace ams::ldr::oc {

#include "mtc_timing_table.hpp"

enum MtcConfig {
    AUTO_ADJ_MARIKO_SAFE_NO_ADJ_ERISTA = 0,
    AUTO_ADJ_MARIKO_4266_NO_ADJ_ERISTA = 1,
    NO_ADJ_ALL = 2,
};

typedef struct __attribute__((packed)) CustomizeTable {
    u8  cust[4] = {'C', 'U', 'S', 'T'};
    u16 custRev = CUST_REV;
    u16 mtcConf = AUTO_ADJ_MARIKO_SAFE_NO_ADJ_ERISTA;
    u32 marikoCpuMaxClock;
    u32 marikoCpuBoostClock;
    u32 marikoCpuMaxVolt;
    u32 marikoGpuMaxClock;
    u32 marikoEmcMaxClock;
    u32 marikoEmcVddqVolt;
    u32 eristaCpuMaxVolt;
    u32 eristaEmcMaxClock;
    u32 commonEmcMemVolt;
} CustomizeTable;

extern volatile CustomizeTable C;

}