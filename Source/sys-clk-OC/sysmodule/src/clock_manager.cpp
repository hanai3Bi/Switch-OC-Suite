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
#include "errors.h"
#include "clock_manager.h"
#include "file_utils.h"
#include "clocks.h"
#include "process_management.h"
#include <cstring>

ClockManager* ClockManager::instance = NULL;

ClockManager* ClockManager::GetInstance()
{
    return instance;
}

void ClockManager::Exit()
{
    if(instance)
    {
        delete instance;
    }
}

void ClockManager::Initialize()
{
    if(!instance)
    {
        instance = new ClockManager();
    }
}

ClockManager::ClockManager()
{
    this->config = Config::CreateDefault();
    this->context = new SysClkContext;
    this->context->applicationId = 0;
    this->context->profile = SysClkProfile_Handheld;
    this->context->realProfile = SysClkProfile_Handheld;
    this->context->enabled = false;
    for(unsigned int i = 0; i < SysClkModule_EnumMax; i++)
    {
        this->context->freqs[i] = 0;
        this->context->overrideFreqs[i] = 0;
    }
    this->context->perfConfId = 0;
    this->running = false;
    this->lastTempLogNs = 0;
    this->lastCsvWriteNs = 0;

    this->oc = new SysClkOcExtra;
    this->oc->systemCoreBoostCPU = false;
    this->oc->gotBoostCPUFreq = false;
    this->oc->allowUnsafeFreq = false;
    this->oc->reverseNXMode = ReverseNX_NotFound;
    this->oc->maxMEMFreq = 0;
    this->oc->boostCPUFreq = 1785'000'000;
}

ClockManager::~ClockManager()
{
    delete this->config;
    delete this->context;
    delete this->oc;
}

bool ClockManager::IsCpuBoostMode()
{
    std::uint32_t confId = this->context->perfConfId;
    bool isCpuBoostMode = (confId == 0x92220009 || confId == 0x9222000A);
    if (isCpuBoostMode && !this->oc->gotBoostCPUFreq)
    {
        this->oc->gotBoostCPUFreq = true;
        this->oc->boostCPUFreq = std::max(this->context->freqs[SysClkModule_CPU], this->oc->boostCPUFreq);
    }
    return isCpuBoostMode;
}

bool ClockManager::IsReverseNXModeValid()
{
    return (this->oc->reverseNXMode);
}

bool ClockManager::IsReverseNXDocked()
{
    return (this->oc->reverseNXMode == ReverseNX_Docked);
}

void ClockManager::SetRunning(bool running)
{
    this->running = running;
}

bool ClockManager::Running()
{
    return this->running;
}

uint32_t ClockManager::GetHz(SysClkModule module)
{
    uint32_t hz = 0;

    /* Temp override setting */
    hz = this->context->overrideFreqs[module];

    /* Global setting */
    if (!hz)
        hz = this->config->GetAutoClockHz(0xA111111111111111, module, this->context->profile);

    /* Per-Game setting */
    if (!hz)
        hz = this->config->GetAutoClockHz(this->context->applicationId, module, this->context->profile);

    /* Return pre-set hz if ReverseNX is enabled, downclock is disabled when realProfile == Docked */
    if (!hz && IsReverseNXModeValid())
    {
        switch(module)
        {
            case SysClkModule_CPU:
                hz = 1020'000'000;
                break;
            case SysClkModule_GPU:
                if (!IsReverseNXDocked() && this->context->realProfile != SysClkProfile_Docked)
                    hz = 460'800'000;
                else
                    hz = 768'000'000;
                break;
            case SysClkModule_MEM:
                if (!IsReverseNXDocked() && this->context->realProfile != SysClkProfile_Docked)
                    hz = 1600000000;
                else
                    hz = MAX_MEM_CLOCK;
                break;
            default:
                break;
        }
    }

    if (hz)
    {
        /* Considering realProfile frequency limit */
        hz = Clocks::GetNearestHz(module, this->context->realProfile, hz, this->oc->allowUnsafeFreq);

        if (module == SysClkModule_MEM && hz == MAX_MEM_CLOCK)
        {
            /* Trigger Max Mem Clock and record it */
            if (!this->oc->maxMEMFreq)
            {
                uint32_t CurrentHz = Clocks::GetCurrentHz(SysClkModule_MEM);
                Clocks::SetHz(SysClkModule_MEM, MAX_MEM_CLOCK);
                this->oc->maxMEMFreq = Clocks::GetCurrentHz(SysClkModule_MEM);
                Clocks::SetHz(SysClkModule_MEM, CurrentHz);
            }

            return this->oc->maxMEMFreq;
        }
    }

    /* Handle CPU Auto Boost, no user-defined hz required */
    if (module == SysClkModule_CPU)
    {
        if (this->oc->systemCoreBoostCPU && hz < this->oc->boostCPUFreq)
            return this->oc->boostCPUFreq;
        else if (!hz)
            /* Prevent crash when hz = 0 in SetHz(0), trigger RefreshContext() and Tick() */
            return 1020'000'000;
    }

    return hz;
}

void ClockManager::Tick()
{
    std::scoped_lock lock{this->contextMutex};

    bool setClock = false;
    setClock |= this->config->Refresh();
    setClock |= this->RefreshContext();
    setClock &= this->context->enabled;
    if (setClock)
    {
        for (unsigned int module = 0; module < SysClkModule_EnumMax; module++)
        {
            uint32_t hz = GetHz((SysClkModule)module);

            if (hz && hz != this->context->freqs[module])
            {
                // Skip setting CPU or GPU clocks in CpuBoostMode if CPU <= boostCPUFreq or GPU >= 76.8MHz
                if (IsCpuBoostMode() && ((module == SysClkModule_CPU && hz <= this->oc->boostCPUFreq) || module == SysClkModule_GPU))
                {
                    continue;
                }

                FileUtils::LogLine("[mgr] %s clock set : %u.%u Mhz", Clocks::GetModuleName((SysClkModule)module, true), hz/1000000, hz/100000 - hz/1000000*10);
                Clocks::SetHz((SysClkModule)module, hz);
                this->context->freqs[module] = hz;
            }
        }
    }
}

void ClockManager::WaitForNextTick()
{
    /* Self-check system core (#3) usage via idleticks at intervals (Not enabled at higher CPU freq or without charger) */
    uint64_t tickWaitTimeMs = this->GetConfig()->GetConfigValue(SysClkConfigValue_PollingIntervalMs);
    uint64_t tickWaitTimeNs = tickWaitTimeMs * 1000000ULL;

    bool isAutoBoostEnabled = this->GetConfig()->GetConfigValue(SysClkConfigValue_AutoCPUBoost);
    if (   isAutoBoostEnabled
        && this->context->realProfile != SysClkProfile_Handheld
        && this->context->enabled
        && this->context->freqs[SysClkModule_CPU] <= this->oc->boostCPUFreq)
    {
        uint64_t systemCoreIdleTickPrev = 0, systemCoreIdleTickNext = 0;
        svcGetInfo(&systemCoreIdleTickPrev, InfoType_IdleTickCount, INVALID_HANDLE, 3);
        svcSleepThread(tickWaitTimeNs);
        svcGetInfo(&systemCoreIdleTickNext, InfoType_IdleTickCount, INVALID_HANDLE, 3);

        /* Convert idletick to free% */
        /* If CPU core usage is 0%, then idletick = 19'200'000 per sec */
        uint64_t systemCoreIdleTick = systemCoreIdleTickNext - systemCoreIdleTickPrev;
        uint64_t freeIdleTick = 19'200 * tickWaitTimeMs;
        uint8_t  freePerc = systemCoreIdleTick / (freeIdleTick / 100);

        constexpr uint8_t systemCoreBoostFreeThreshold = 5;

        bool systemCoreBoostCPUPrevState = this->oc->systemCoreBoostCPU;
        this->oc->systemCoreBoostCPU = (freePerc <= systemCoreBoostFreeThreshold);

        if (systemCoreBoostCPUPrevState && !this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, GetHz(SysClkModule_CPU));
        }
        else if (!systemCoreBoostCPUPrevState && this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, this->oc->boostCPUFreq);
        }
    }
    else
    {
        if (this->oc->systemCoreBoostCPU) {
            this->oc->systemCoreBoostCPU = false;
            Clocks::SetHz(SysClkModule_CPU, GetHz(SysClkModule_CPU));
        }
        svcSleepThread(tickWaitTimeNs);
    }
}

SysClkProfile ClockManager::ReverseNXProfileHandler()
{
    if (!IsReverseNXModeValid())
    {
        return this->context->realProfile;
    }

    if (IsReverseNXDocked())
    {
        return SysClkProfile_Docked;
    }

    if (// !IsReversedNXDocked() &&
           this->context->realProfile == SysClkProfile_Docked)
    {
        return SysClkProfile_HandheldChargingOfficial;
    }

    return this->context->realProfile;
}

ReverseNXMode ClockManager::ReverseNXFileHandler(const char* filePath)
{
    FILE *readFile;
    readFile = fopen(filePath, "rb");

    if (!readFile)
        return ReverseNX_NotFound;

    uint64_t magicDocked   = 0xD65F03C0320003E0;
    uint64_t magicHandheld = 0xD65F03C052A00000;

    uint64_t readBuffer = 0;
    fread(&readBuffer, 1, sizeof(readBuffer), readFile);
    fclose(readFile);

    if (!memcmp(&readBuffer, &magicDocked, sizeof(readBuffer)))
        return ReverseNX_Docked;

    if (!memcmp(&readBuffer, &magicHandheld, sizeof(readBuffer)))
        return ReverseNX_Handheld;

    return ReverseNX_NotValid;
}

void ClockManager::CheckReverseNXTool()
{
    bool shouldCheckReverseNXTool = FileUtils::ExistReverseNXTool();
    if (!shouldCheckReverseNXTool)
        return;

    ReverseNXMode getMode = ReverseNX_NotValid;
    if (this->context->applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
    {
        const char asmFileName[] = "_ZN2nn2oe18GetPerformanceModeEv.asm64"; // Checking one asm64 file is enough
        char asmFilePath[128];

        /* Check global override */
        snprintf(asmFilePath, sizeof(asmFilePath), "/SaltySD/patches/%s", asmFileName);
        getMode = ReverseNXFileHandler(asmFilePath);

        if (!getMode)
        {
            /* Check per-game override */
            snprintf(asmFilePath, sizeof(asmFilePath), "/SaltySD/patches/%016lX/%s", this->context->applicationId, asmFileName);
            getMode = ReverseNXFileHandler(asmFilePath);
        }
    }

    this->oc->reverseNXMode = getMode;
}

bool ClockManager::CheckReverseNXRT()
{
    bool shouldAdjustProfile = false;

    ReverseNXMode getMode = this->GetConfig()->GetReverseNXRTMode();
    if (getMode)
    {
        this->GetConfig()->SetReverseNXRTMode(ReverseNX_GotValue);
        this->oc->reverseNXMode = (getMode == ReverseNX_RTResetToDefault) ?
                                    ReverseNX_SystemDefault : getMode;
        shouldAdjustProfile = true;
    }

    return shouldAdjustProfile;
}

void ClockManager::ChargingHandler()
{
    smInitialize();
    psmInitialize();
    ChargeInfo* chargeInfoField = new ChargeInfo;
    Service* session = psmGetServiceSession();
    serviceDispatchOut(session, GetBatteryChargeInfoFields, *(chargeInfoField));

    bool fastChargingEnabled = chargeInfoField->ChargeCurrentLimit > 768;
    bool fastChargingConfig  = !(this->GetConfig()->GetConfigValue(SysClkConfigValue_DisableFastCharging));
    if (fastChargingEnabled != fastChargingConfig)
    {
        serviceDispatch(session, fastChargingConfig ? EnableFastBatteryCharging : DisableFastBatteryCharging);
    }

    // bool isChargerConnected = (chargeInfoField->ChargerType != ChargerType_None);
    // if (isChargerConnected)
    // {
    //     u32 chargeNow = 0;

    //     if (R_SUCCEEDED(psmGetBatteryChargePercentage(&chargeNow)))
    //     {
    //         bool isCharging = ((chargeInfoField->unk_x14 >> 8) & 1);
    //         u32 chargeLimit = this->GetConfig()->GetConfigValue(SysClkConfigValue_ChargingLimitPercentage);
    //         if (isCharging && chargeLimit < chargeNow) {
    //             serviceDispatch(session, DisableBatteryCharging);
    //         } else if (!isCharging && chargeNow < 100 && chargeLimit > chargeNow) {
    //             serviceDispatch(session, EnableBatteryCharging);
    //         }
    //     }
    // }

    delete chargeInfoField;
    psmExit();
    smExit();
}

bool ClockManager::RefreshContext()
{
    ChargingHandler();

    bool hasChanged = false;
    bool isReverseNXSyncEnabled = this->GetConfig()->GetConfigValue(SysClkConfigValue_SyncReverseNXMode);
    this->oc->allowUnsafeFreq = this->GetConfig()->GetConfigValue(SysClkConfigValue_AllowUnsafeFrequencies);

    bool enabled = this->GetConfig()->Enabled();
    if(enabled != this->context->enabled)
    {
        this->context->enabled = enabled;
        FileUtils::LogLine("[mgr] " TARGET " status: %s", enabled ? "enabled" : "disabled");
        hasChanged = true;
    }

    std::uint64_t applicationId = ProcessManagement::GetCurrentApplicationId();
    if (applicationId != this->context->applicationId)
    {
        FileUtils::LogLine("[mgr] TitleID change: %016lX", applicationId);
        this->context->applicationId = applicationId;
        hasChanged = true;

        /* Clear ReverseNX state and recheck -Tool patches*/
        if (isReverseNXSyncEnabled) {
            this->oc->reverseNXMode = ReverseNX_SystemDefault;
            CheckReverseNXTool();
        }
    }

    SysClkProfile profile = Clocks::GetCurrentProfile();
    if (profile != this->context->realProfile)
    {
        FileUtils::LogLine("[mgr] Profile change: %s", Clocks::GetProfileName(profile, true));
        this->context->realProfile = profile;
        hasChanged = true;
    }

    /* Update PerformanceConfigurationId */
    {
        uint32_t confId = 0;
        Result rc = 0;
        rc = apmExtGetCurrentPerformanceConfiguration(&confId);
        ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");
        if (this->context->perfConfId != confId)
        {
            this->context->perfConfId = confId;
            hasChanged = true;
        }
    }

    // restore clocks to stock values on app or profile change
    // and let ptm module handle boost clocks rather than resetting
    if (hasChanged && !IsCpuBoostMode())
        Clocks::ResetToStock();

    /* Check ReverseNX-RT and adjust nominal profile when context changes */
    if (isReverseNXSyncEnabled) {
        hasChanged |= CheckReverseNXRT();
        if (hasChanged)
            this->context->profile = ReverseNXProfileHandler();
    }

    std::uint32_t hz = 0;
    for (unsigned int module = 0; module < SysClkModule_EnumMax; module++)
    {
        hz = Clocks::GetCurrentHz((SysClkModule)module);

        if (hz != 0 && hz != this->context->freqs[module])
        {
            FileUtils::LogLine("[mgr] %s clock change: %u.%u Mhz", Clocks::GetModuleName((SysClkModule)module, true), hz/1000000, hz/100000 - hz/1000000*10);
            this->context->freqs[module] = hz;
            hasChanged = true;
        }

        hz = this->GetConfig()->GetOverrideHz((SysClkModule)module);
        if (hz != this->context->overrideFreqs[module])
        {
            if(hz)
            {
                FileUtils::LogLine("[mgr] %s override change: %u.%u Mhz", Clocks::GetModuleName((SysClkModule)module, true), hz/1000000, hz/100000 - hz/1000000*10);
            }
            else
            {
                FileUtils::LogLine("[mgr] %s override disabled", Clocks::GetModuleName((SysClkModule)module, true));
                Clocks::ResetToStock(module);
            }
            this->context->overrideFreqs[module] = hz;
            hasChanged = true;
        }
    }

    // temperatures do not and should not force a refresh, hasChanged untouched
    std::uint32_t millis = 0;
    std::uint64_t ns = armTicksToNs(armGetSystemTick());
    std::uint64_t tempLogInterval = this->GetConfig()->GetConfigValue(SysClkConfigValue_TempLogIntervalMs) * 1000000ULL;
    bool shouldLogTemp = tempLogInterval && ((ns - this->lastTempLogNs) > tempLogInterval);
    for (unsigned int sensor = 0; sensor < SysClkThermalSensor_EnumMax; sensor++)
    {
        millis = Clocks::GetTemperatureMilli((SysClkThermalSensor)sensor);
        if(shouldLogTemp)
        {
            FileUtils::LogLine("[mgr] %s temp: %u.%u °C", Clocks::GetThermalSensorName((SysClkThermalSensor)sensor, true), millis/1000, (millis - millis/1000*1000) / 100);
        }
        this->context->temps[sensor] = millis;
    }

    if(shouldLogTemp)
    {
        this->lastTempLogNs = ns;
    }

    std::uint64_t csvWriteInterval = this->GetConfig()->GetConfigValue(SysClkConfigValue_CsvWriteIntervalMs) * 1000000ULL;

    if(csvWriteInterval && ((ns - this->lastCsvWriteNs) > csvWriteInterval))
    {
        FileUtils::WriteContextToCsv(this->context);
        this->lastCsvWriteNs = ns;
    }

    return hasChanged;
}

SysClkContext ClockManager::GetCurrentContext()
{
    std::scoped_lock lock{this->contextMutex};
    return *this->context;
}

Config* ClockManager::GetConfig()
{
    return this->config;
}
