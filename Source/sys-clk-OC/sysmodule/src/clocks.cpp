/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include <cstring>
#include <nxExt.h>
#include "clocks.h"
#include "errors.h"
#include "file_utils.h"

void Clocks::GetRange(SysClkModule module, SysClkProfile profile, uint32_t** min, uint32_t** max)
{
    if (module == SysClkModule_MEM) {
        *min = isMariko ? &g_freq_table_mem_hz[4] : &g_freq_table_mem_hz[0]; // 1600 / 665
        *max = &g_freq_table_mem_hz[5]; // 1862 - Max
        return;
    }

    if (module == SysClkModule_CPU) {
        *min = &g_freq_table_cpu_hz[0];
        if (isMariko)
            *max = !allowUnsafe ? &g_freq_table_cpu_hz[15] : &g_freq_table_cpu_hz[19]; // 1963 / 2397
        else
            *max = (!allowUnsafe || profile == SysClkProfile_Handheld) ?
                    &g_freq_table_cpu_hz[13] : &g_freq_table_cpu_hz[16]; // 1785 / 2091

        return;
    }

    if (module == SysClkModule_GPU) {
        *min = &g_freq_table_gpu_hz[0];
        if (isMariko) {
            *max = (!allowUnsafe || profile == SysClkProfile_Handheld) ?
                    &g_freq_table_gpu_hz[12] : &g_freq_table_gpu_hz[17]; // 998 / 1305
            return;
        }

        switch (profile) {
            case SysClkProfile_Handheld:
                *max = &g_freq_table_gpu_hz[5]; // 460
                break;
            case SysClkProfile_HandheldChargingUSB:
                *max = &g_freq_table_gpu_hz[9]; // 768
                break;
            default:
                *max = &g_freq_table_gpu_hz[11]; // 921
                break;
        }
        return;
    }

    ERROR_THROW("No such PcvModule: %u", module);
}

Result Clocks::GetTable(SysClkModule module, SysClkProfile profile, SysClkFrequencyTable* out_table) {
    uint32_t* min = NULL;
    uint32_t* max = NULL;
    GetRange(module, profile, &min, &max);
    if (!min || !max || (max - min) / sizeof(uint32_t) >= sizeof(SysClkFrequencyTable) / sizeof(uint32_t))
        return 1;

    memset(out_table, 0, sizeof(SysClkFrequencyTable));
    uint32_t* p = min;
    size_t idx = 0;
    while(p <= max)
        out_table->values[idx++] = *p++;

    return 0;
}

void Clocks::Initialize()
{
    Result rc = 0;

    u64 hardware_type = 0;
    rc = splInitialize();
    ASSERT_RESULT_OK(rc, "splInitialize");
    rc = splGetConfig(SplConfigItem_HardwareType, &hardware_type);
    ASSERT_RESULT_OK(rc, "splGetConfig");
    splExit();

    switch (hardware_type) {
        case 0: // Icosa
        case 1: // Copper
            isMariko = false;
            break;
        case 2: // Hoag
        case 3: // Iowa
        case 4: // Calcio
        case 5: // Aula
            isMariko = true;
            break;
        default:
            ERROR_THROW("Unknown hardware type: 0x%X!", hardware_type);
            return;
    }

    if(hosversionAtLeast(8,0,0))
    {
        rc = clkrstInitialize();
        ASSERT_RESULT_OK(rc, "pcvInitialize");
    }
    else
    {
        rc = pcvInitialize();
        ASSERT_RESULT_OK(rc, "pcvInitialize");
    }

    rc = apmExtInitialize();
    ASSERT_RESULT_OK(rc, "apmExtInitialize");

    rc = psmInitialize();
    ASSERT_RESULT_OK(rc, "psmInitialize");

    rc = tsInitialize();
    ASSERT_RESULT_OK(rc, "tsInitialize");

    if(hosversionAtLeast(5,0,0))
    {
        rc = tcInitialize();
        ASSERT_RESULT_OK(rc, "tcInitialize");
    }

    FileUtils::ParseLoaderKip();

    if (!maxMemFreq) {
        uint32_t curr_mem_hz = GetCurrentHz(SysClkModule_MEM);
        SetHz(SysClkModule_MEM, MAX_MEM_CLOCK);
        svcSleepThread(1'000'000);
        if (uint32_t hz = GetCurrentHz(SysClkModule_MEM))
            maxMemFreq = hz;
        else
            maxMemFreq = MAX_MEM_CLOCK;
        SetHz(SysClkModule_MEM, curr_mem_hz);
    }
}

void Clocks::Exit()
{
    if(hosversionAtLeast(8,0,0))
    {
        pcvExit();
    }
    else
    {
        clkrstExit();
    }

    apmExtExit();
    psmExit();
    tsExit();

    if(hosversionAtLeast(5,0,0))
    {
        tcExit();
    }
}

const char* Clocks::GetModuleName(SysClkModule module, bool pretty)
{
    const char* result = sysclkFormatModule(module, pretty);

    if(!result)
    {
        ERROR_THROW("No such SysClkModule: %u", module);
    }

    return result;
}

const char* Clocks::GetProfileName(SysClkProfile profile, bool pretty)
{
    const char* result = sysclkFormatProfile(profile, pretty);

    if(!result)
    {
        ERROR_THROW("No such SysClkProfile: %u", profile);
    }

    return result;
}

const char* Clocks::GetThermalSensorName(SysClkThermalSensor sensor, bool pretty)
{
    const char* result = sysclkFormatThermalSensor(sensor, pretty);

    if(!result)
    {
        ERROR_THROW("No such SysClkThermalSensor: %u", sensor);
    }

    return result;
}

PcvModule Clocks::GetPcvModule(SysClkModule sysclkModule)
{
    switch(sysclkModule)
    {
        case SysClkModule_CPU:
            return PcvModule_CpuBus;
        case SysClkModule_GPU:
            return PcvModule_GPU;
        case SysClkModule_MEM:
            return PcvModule_EMC;
        default:
            ERROR_THROW("No such SysClkModule: %u", sysclkModule);
    }

    return (PcvModule)0;
}

PcvModuleId Clocks::GetPcvModuleId(SysClkModule sysclkModule)
{
    PcvModuleId pcvModuleId;
    Result rc = pcvGetModuleId(&pcvModuleId, GetPcvModule(sysclkModule));
    ASSERT_RESULT_OK(rc, "pcvGetModuleId");

    return pcvModuleId;
}

SysClkApmConfiguration* Clocks::GetEmbeddedApmConfig(uint32_t confId)
{
    SysClkApmConfiguration* apmConfiguration = NULL;
    for(size_t i = 0; sysclk_g_apm_configurations[i].id; i++)
    {
        if(sysclk_g_apm_configurations[i].id == confId)
        {
            apmConfiguration = &sysclk_g_apm_configurations[i];
            break;
        }
    }

    if(!apmConfiguration)
    {
        ERROR_THROW("Unknown apm configuration: %x", confId);
    }
    return apmConfiguration;
}

uint32_t Clocks::GetStockClock(SysClkApmConfiguration* apm, SysClkModule module)
{
    switch (module) {
        case SysClkModule_CPU:
            return apm->cpu_hz;
        case SysClkModule_GPU:
            return apm->gpu_hz;
        case SysClkModule_MEM:
            return GetIsMariko() ? MEM_CLOCK_MARIKO_MIN : apm->mem_hz;
        default:
            ERROR_THROW("Unknown SysClkModule: %x", module);
            return 0;
    }
}

void Clocks::ResetToStock(unsigned int module)
{
    if(hosversionAtLeast(9,0,0))
    {
        std::uint32_t confId = 0;
        Result rc = apmExtGetCurrentPerformanceConfiguration(&confId);
        ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");

        SysClkApmConfiguration* apmConfiguration = GetEmbeddedApmConfig(confId);

        if (module == SysClkModule_EnumMax || module == SysClkModule_CPU)
        {
            Clocks::SetHz(SysClkModule_CPU, GetStockClock(apmConfiguration, SysClkModule_CPU));
        }
        if (module == SysClkModule_EnumMax || module == SysClkModule_GPU)
        {
            Clocks::SetHz(SysClkModule_GPU, GetStockClock(apmConfiguration, SysClkModule_GPU));
        }
        if (module == SysClkModule_EnumMax || module == SysClkModule_MEM)
        {
            Clocks::SetHz(SysClkModule_MEM, GetStockClock(apmConfiguration, SysClkModule_MEM));
        }
    }
    else
    {
        Result rc = 0;
        std::uint32_t mode = 0;
        rc = apmExtGetPerformanceMode(&mode);
        ASSERT_RESULT_OK(rc, "apmExtGetPerformanceMode");

        rc = apmExtSysRequestPerformanceMode(mode);
        ASSERT_RESULT_OK(rc, "apmExtSysRequestPerformanceMode");
    }
}

SysClkProfile Clocks::GetCurrentProfile()
{
    std::uint32_t mode = 0;
    Result rc = apmExtGetPerformanceMode(&mode);
    ASSERT_RESULT_OK(rc, "apmExtGetPerformanceMode");

    if(mode)
    {
        return SysClkProfile_Docked;
    }

    PsmChargerType chargerType;

    rc = psmGetChargerType(&chargerType);
    ASSERT_RESULT_OK(rc, "psmGetChargerType");

    switch(chargerType)
    {
        case PsmChargerType_EnoughPower:
            return SysClkProfile_HandheldChargingOfficial;
        case PsmChargerType_LowPower:
        case PsmChargerType_NotSupported:
            return SysClkProfile_HandheldChargingUSB;
        default:
            return SysClkProfile_Handheld;
    }
}

void Clocks::SetHz(SysClkModule module, std::uint32_t hz)
{
    Result rc = 0;

    if(hosversionAtLeast(8,0,0))
    {
        ClkrstSession session = {0};

        rc = clkrstOpenSession(&session, Clocks::GetPcvModuleId(module), 3);
        ASSERT_RESULT_OK(rc, "clkrstOpenSession");
        rc = clkrstSetClockRate(&session, hz);
        ASSERT_RESULT_OK(rc, "clkrstSetClockRate");

        clkrstCloseSession(&session);
    }
    else
    {
        rc = pcvSetClockRate(Clocks::GetPcvModule(module), hz);
        ASSERT_RESULT_OK(rc, "pcvSetClockRate");
    }
}

std::uint32_t Clocks::GetCurrentHz(SysClkModule module)
{
    Result rc = 0;
    std::uint32_t hz = 0;

    if(hosversionAtLeast(8,0,0))
    {
        ClkrstSession session = {0};

        rc = clkrstOpenSession(&session, Clocks::GetPcvModuleId(module), 3);
        ASSERT_RESULT_OK(rc, "clkrstOpenSession");

        rc = clkrstGetClockRate(&session, &hz);
        ASSERT_RESULT_OK(rc, "clkrstGetClockRate");

        clkrstCloseSession(&session);
    }
    else
    {
        rc = pcvGetClockRate(Clocks::GetPcvModule(module), &hz);
        ASSERT_RESULT_OK(rc, "pcvGetClockRate");
    }

    return hz;
}

std::uint32_t Clocks::GetNearestHz(SysClkModule module, SysClkProfile profile, std::uint32_t inHz)
{
    uint32_t inMHz = inHz / 1000000U;
    if (module == SysClkModule_MEM && inMHz == MAX_MEM_CLOCK / 1000'000)
        return Clocks::maxMemFreq;

    uint32_t* min = NULL;
    uint32_t* max = NULL;
    GetRange(module, profile, &min, &max);

    if (!min || !max)
        ERROR_THROW("table lookup failed for SysClkModule: %u", module);

    uint32_t* p = min;
    while(p <= max) {
        if (inMHz == *p / 1000000U)
            return *p;
        p++;
    }

    return *max;
}

std::int32_t Clocks::GetTsTemperatureMilli(TsLocation location)
{
    Result rc;
    std::int32_t millis = 0;

    if(hosversionAtLeast(14,0,0))
    {
        rc = tsGetTemperature(location, &millis);
        ASSERT_RESULT_OK(rc, "tsGetTemperature(%u)", location);
        millis *= 1000;
    }
    else
    {
        rc = tsGetTemperatureMilliC(location, &millis);
        ASSERT_RESULT_OK(rc, "tsGetTemperatureMilliC(%u)", location);
    }

    return millis;
}

std::uint32_t Clocks::GetTemperatureMilli(SysClkThermalSensor sensor)
{
    std::int32_t millis = 0;

    if(sensor == SysClkThermalSensor_SOC)
    {
        millis = GetTsTemperatureMilli(TsLocation_External);
    }
    else if(sensor == SysClkThermalSensor_PCB)
    {
        millis = GetTsTemperatureMilli(TsLocation_Internal);
    }
    else if(sensor == SysClkThermalSensor_Skin)
    {
        if(hosversionAtLeast(5,0,0))
        {
            Result rc = tcGetSkinTemperatureMilliC(&millis);
            ASSERT_RESULT_OK(rc, "tcGetSkinTemperatureMilliC");
        }
    }
    else
    {
        ERROR_THROW("No such SysClkThermalSensor: %u", sensor);
    }

    return std::max(0, millis);
}