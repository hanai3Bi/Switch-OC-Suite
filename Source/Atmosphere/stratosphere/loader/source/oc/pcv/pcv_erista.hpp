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

#include "pcv.hpp"

namespace ams::ldr::oc::pcv::erista {

constexpr cpu_freq_cvb_table_t CpuCvbTableDefault[] = {
    // CPU_PLL_CVB_TABLE_ODN
    {  204000, {  721094 }, {} },
    {  306000, {  754040 }, {} },
    {  408000, {  786986 }, {} },
    {  510000, {  819932 }, {} },
    {  612000, {  852878 }, {} },
    {  714000, {  885824 }, {} },
    {  816000, {  918770 }, {} },
    {  918000, {  951716 }, {} },
    { 1020000, {  984662 }, { -2875621,  358099, -8585 } },
    { 1122000, { 1017608 }, {   -52225,  104159, -2816 } },
    { 1224000, { 1050554 }, {  1076868,    8356,  -727 } },
    { 1326000, { 1083500 }, {  2208191,  -84659,  1240 } },
    { 1428000, { 1116446 }, {  2519460, -105063,  1611 } },
    { 1581000, { 1130000 }, {  2889664, -122173,  1834 } },
    { 1683000, { 1168000 }, {  5100873, -279186,  4747 } },
    { 1785000, { 1227500 }, {  5100873, -279186,  4747 } },
};

constexpr u32 CpuVoltL4T = 1235'000;

constexpr cpu_freq_cvb_table_t CpuCvbTableAppend[] = {
    { 1887000, { CpuVoltL4T }, {  5100873, -279186,  4747 } },
    { 1963500, { CpuVoltL4T }, {  5100873, -279186,  4747 } },
    { 2091000, { CpuVoltL4T }, {  5100873, -279186,  4747 } },
};

constexpr u32 CpuMinVolts[] = { 950, 850, 825, 810 };

inline bool CpuMaxVoltPatternFn(u32* ptr32) {
    u32 val = *ptr32;
    return (val == 1132 || val == 1170 || val == 1227);
}

constexpr u32 MemVoltHOS      = 1125'000;
constexpr u32 EmcClkPllmLimit = 1866'000'000;

constexpr u32 MTC_TABLE_REV = 7;

void Patch(uintptr_t mapped_nso, size_t nso_size);

}