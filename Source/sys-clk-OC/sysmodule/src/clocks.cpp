/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include <nxExt.h>
#include "clocks.h"
#include "errors.h"
#include "file_utils.h"

bool Clocks::isMariko = false;

typedef enum {
    None         = 0,
    PD           = 1,
    TypeC_1500mA = 2,
    TypeC_3000mA = 3,
    DCP          = 4,
    CDP          = 5,
    SDP          = 6,
    Apple_500mA  = 7,
    Apple_1000mA = 8,
    Apple_2000mA = 9
} BatteryChargeInfoFieldsChargerType;

typedef struct {
    uint32_t InputCurrentLimit;     //Input (Sink) current limit in mA
    uint32_t VBUSCurrentLimit;      //Output (Source/VBUS/OTG) current limit in mA
    uint32_t ChargeCurrentLimit;    //Battery charging current limit in mA (512mA when Docked, 768mA when BatteryTemperature < 17.0 C)
    uint32_t ChargeVoltageLimit;    //Battery charging voltage limit in mV (3952mV when BatteryTemperature >= 51.0 C)
    uint32_t unk_x10;               //Possibly an emum, getting the same value as PowerRole in all tested cases
    uint32_t unk_x14;               //Possibly flags
    uint16_t PDControllerState;     //Power Delivery Controller State
    uint32_t BatteryTemperature;    //Battery temperature in milli C
    uint32_t RawBatteryCharge;      //Raw battery charged capacity per cent-mille (i.e. 100% = 100000 pcm)
    uint32_t VoltageAvg;            //Voltage avg in mV (more in Notes)
    uint32_t BatteryAge;            //Battery age (capacity full / capacity design) per cent-mille (i.e. 100% = 100000 pcm)
    uint16_t PowerRole;             //Sink or Source
    BatteryChargeInfoFieldsChargerType ChargerType;
    uint32_t ChargerVoltageLimit;   //Charger and external device voltage limit in mV
    uint32_t ChargerCurrentLimit;   //Charger and external device current limit in mA
    uint16_t Flags;                 //Unknown flags
} BatteryChargeInfoFields;

void Clocks::GetList(SysClkModule module, std::uint32_t **outClocks)
{
    switch(module)
    {
        case SysClkModule_CPU:
            *outClocks = sysclk_g_freq_table_cpu_hz;
            break;
        case SysClkModule_GPU:
            *outClocks = sysclk_g_freq_table_gpu_hz;
            break;
        case SysClkModule_MEM:
            *outClocks = sysclk_g_freq_table_mem_hz;
            break;
        default:
            *outClocks = NULL;
            ERROR_THROW("No such PcvModule: %u", module);
    }
}

void Clocks::Initialize()
{
    Result rc = 0;

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

void Clocks::ResetToStock(unsigned int module)
{
    Result rc = 0;
    if(hosversionAtLeast(9,0,0))
    {
        std::uint32_t confId = 0;
        rc = apmExtGetCurrentPerformanceConfiguration(&confId);
        ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");

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

        if (module == SysClkModule_EnumMax || module == SysClkModule_CPU)
        {
            Clocks::SetHz(SysClkModule_CPU, apmConfiguration->cpu_hz);
        }
        if (module == SysClkModule_EnumMax || module == SysClkModule_GPU)
        {
            Clocks::SetHz(SysClkModule_GPU, apmConfiguration->gpu_hz);
        }
        // We don't need to set MEM freqs any more
        //Clocks::SetHz(SysClkModule_MEM, apmConfiguration->mem_hz);
    }
    else
    {
        std::uint32_t mode = 0;
        rc = apmExtGetPerformanceMode(&mode);
        ASSERT_RESULT_OK(rc, "apmExtGetPerformanceMode");

        rc = apmExtSysRequestPerformanceMode(mode);
        ASSERT_RESULT_OK(rc, "apmExtSysRequestPerformanceMode");
    }
}

Result psmGetBatteryChargeInfoFields(Service* psmService, BatteryChargeInfoFields *out) {
    return serviceDispatchOut(psmService, 17, *out);
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

    if(FileUtils::IsPd18wAsOfficialChargerEnabled())
    {
        static Service* psmService = psmGetServiceSession();
        static BatteryChargeInfoFields* _Fields = new BatteryChargeInfoFields;
        psmGetBatteryChargeInfoFields(psmService, _Fields);
        if(_Fields->ChargerType == PD)
        {
            uint8_t ChargerWatts = (_Fields->ChargerVoltageLimit * _Fields->ChargerCurrentLimit)/1000000.0;
            if(ChargerWatts >= 17)
                return SysClkProfile_HandheldChargingOfficial;
        }
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
    // We don't need to set MEM freqs any more
    if (module == SysClkModule_MEM)
        return;

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
    std::uint32_t hz = GetNearestHz(module, inHz);
    std::uint32_t maxHz = GetMaxAllowedHz(module, profile);

    if(maxHz != 0)
    {
        hz = std::min(hz, maxHz);
    }

    return hz;
}

std::uint32_t Clocks::GetMaxAllowedHz(SysClkModule module, SysClkProfile profile)
{
    if(module == SysClkModule_GPU)
    {
        if(profile < SysClkProfile_HandheldCharging)
        {
            return isMariko ? 1536000000 : SYSCLK_GPU_HANDHELD_MAX_HZ;
        }
        else if(profile <= SysClkProfile_HandheldChargingUSB)
        {
            return isMariko ? 1536000000 : SYSCLK_GPU_UNOFFICIAL_CHARGER_MAX_HZ;
        }
    }

    return 0;
}

std::uint32_t Clocks::GetNearestHz(SysClkModule module, std::uint32_t inHz)
{
    // Hardcoded values to return, I don't know why it will bump to max when excessive OC
    if(module == SysClkModule_MEM)
    {
        switch(inHz)
        {
            // From Hekate Minerva module
            case 1331000000:
                return 1331200000;
            case 1795000000:
                return 1795200000;
            case 1862000000:
                return 1862400000;
            case 1894000000:
                return 1894400000;
            case 1932000000:
                return 1932800000;
            case 1996000000:
                return 1996800000;
            case 2099000000:
                return 2099200000;
            case 2131000000:
                return 2131200000;
            default:
                return inHz;
        }
    }

    if(module == SysClkModule_CPU)
    {
        switch(inHz)
        {
            case 1963000000:
                return 1963500000;
            default:
                return inHz;
        }
    }

    if(module == SysClkModule_GPU)
    {
        switch(inHz)
        {
            case 76000000:
                return 76800000;
            case 153000000:
                return 153600000;
            case 230000000:
                return 230400000;
            case 307000000:
                return 307200000;
            case 460000000:
                return 460800000;
            case 537000000:
                return 537600000;
            case 614000000:
                return 614400000;
            case 691000000:
                return 691200000;
            case 844000000:
                return 844800000;
            case 921000000:
                return 921600000;
            case 998000000:
                return 998400000;
            case 1075000000:
                return 1075200000;
            case 1228000000:
                return 1228800000;
            case 1267000000:
                return 1267200000;
            case 1305000000:
                return 1305600000;
            case 1382000000:
                return 1382400000;
            case 1420000000:
                return 1420800000;
            default:
                return inHz;
        }
    }

    return inHz;
    std::uint32_t *clockTable = NULL;
    GetList(module, &clockTable);

    if (!clockTable || !clockTable[0])
    {
        ERROR_THROW("table lookup failed for SysClkModule: %u", module);
    }

    int i = 0;
    while(clockTable[i + 1])
    {
        if (inHz <= (clockTable[i] + clockTable[i + 1]) / 2)
        {
            break;
        }
        i++;
    }

    return clockTable[i];
}

std::uint32_t Clocks::GetTemperatureMilli(SysClkThermalSensor sensor)
{
    Result rc;
    std::int32_t millis = 0;

    if(sensor == SysClkThermalSensor_SOC)
    {
        rc = tsGetTemperatureMilliC(TsLocation_External, &millis);
        ASSERT_RESULT_OK(rc, "tsGetTemperatureMilliC");
    }
    else if(sensor == SysClkThermalSensor_PCB)
    {
        rc = tsGetTemperatureMilliC(TsLocation_Internal, &millis);
        ASSERT_RESULT_OK(rc, "tsGetTemperatureMilliC");
    }
    else if(sensor == SysClkThermalSensor_Skin)
    {
        if(hosversionAtLeast(5,0,0))
        {
            rc = tcGetSkinTemperatureMilliC(&millis);
            ASSERT_RESULT_OK(rc, "tcGetSkinTemperatureMilliC");
        }
    }
    else
    {
        ERROR_THROW("No such SysClkThermalSensor: %u", sensor);
    }

    return std::max(0, millis);
}