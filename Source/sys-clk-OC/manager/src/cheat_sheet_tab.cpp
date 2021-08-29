/*
    sys-clk manager, a sys-clk frontend homebrew
    Copyright (C) 2019-2020  natinusala
    Copyright (C) 2019  p-sam
    Copyright (C) 2019  m4xw

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cheat_sheet_tab.h"
#include "ipc/client.h"
#include "utils.h"

#include <borealis.hpp>

CheatSheetTab::CheatSheetTab()
{
    // CPU
    this->addView(new brls::Header("CPU Clocks"));
    brls::Table *cpuTable = new brls::Table();

    cpuTable->addRow(brls::TableRowType::BODY, "OC Suite Maximum", "2397.0 MHz");
    cpuTable->addRow(brls::TableRowType::BODY, "Official Boost", "1785.0 MHz");
    cpuTable->addRow(brls::TableRowType::BODY, "Official Docked and Handheld", "1020.0 MHz");

    this->addView(cpuTable);

    // GPU
    this->addView(new brls::Header("GPU Clocks"));
    brls::Table *gpuTable = new brls::Table();

    gpuTable->addRow(brls::TableRowType::BODY, "OC Suite Maximum", "1344.0 MHz");
    gpuTable->addRow(brls::TableRowType::BODY, "Official Maximum", "921.6 MHz");
    gpuTable->addRow(brls::TableRowType::BODY, "Official Docked", "768.0 MHz");
    gpuTable->addRow(brls::TableRowType::BODY, "Official Handheld", "384.0/460.8 MHz");

    this->addView(gpuTable);

    // MEM
    this->addView(new brls::Header("MEM Clocks"));
    brls::Table *memTable = new brls::Table();

    // Get context
    SysClkContext context;
    Result rc = sysclkIpcGetCurrentContext(&context);

    if (R_FAILED(rc))
    {
        brls::Logger::error("Unable to get context");
        errorResult("sysclkIpcGetCurrentContext", rc);
        brls::Application::crash("Could not get the current sys-clk context, please check that it is correctly installed and enabled.");
        return;
    }

    memTable->addRow(brls::TableRowType::BODY, "OC Suite", formatFreq(context.freqs[SysClkModule_MEM]));
    memTable->addRow(brls::TableRowType::BODY, "Official", "1331.2/1600.0 MHz");

    this->addView(memTable);
}

void CheatSheetTab::customSpacing(brls::View* current, brls::View* next, int* spacing)
{
    if (dynamic_cast<brls::Table*>(current))
        *spacing = 0;
    else
        List::customSpacing(current, next, spacing);
}
