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
    this->oc->systemCoreCheckStuck = false;
    this->oc->reverseNXToolMode = ReverseNX_NotFound;
    this->oc->reverseNXRTMode = ReverseNX_NotFound;
    this->oc->tickWaitTimeMs = 0;
    this->oc->maxMEMFreq = 1600'000'000;
    // this->oc->systemCoreStuckCount = 0;
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
    return (confId == 0x92220009 || confId == 0x9222000A);
}

bool ClockManager::IsReverseNXEnabled()
{
    return (this->oc->reverseNXRTMode || this->oc->reverseNXToolMode);
}

bool ClockManager::IsReverseNXDocked()
{
    return (this->oc->reverseNXRTMode == ReverseNX_Docked
         || this->oc->reverseNXToolMode == ReverseNX_Docked);
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
    if (!hz && IsReverseNXEnabled())
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
                    hz = 1331'200'000;
                else
                    hz = 1600'000'000;
                break;
            default:
                break;
        }
    }

    if (hz)
    {
        /* Considering realProfile frequency limit */
        hz = Clocks::GetNearestHz(module, this->context->realProfile, hz);

        if (module == SysClkModule_MEM && hz == 1600'000'000)
        {
            /* Return maxMemFreq */
            if (this->context->freqs[module] > this->oc->maxMEMFreq)
                this->oc->maxMEMFreq = this->context->freqs[module];

            return this->oc->maxMEMFreq;
        }
    }

    if (module == SysClkModule_CPU)
    {
        if (this->oc->systemCoreBoostCPU && hz < CPU_BOOST_FREQ)
            return CPU_BOOST_FREQ;
        else if (!hz)
            /* Prevent crash when hz = 0 in SetHz(0), trigger RefreshContext() and Tick() */
            return 1020'000'000;
    }

    return hz;
}

void ClockManager::Tick()
{
    std::scoped_lock lock{this->contextMutex};

    if ((this->RefreshContext() || this->config->Refresh()) && this->context->enabled)
    {
        for (unsigned int module = 0; module < SysClkModule_EnumMax; module++)
        {
            uint32_t hz = GetHz((SysClkModule)module);

            if (hz && hz != this->context->freqs[module])
            {
                // Skip setting CPU or GPU clocks in CpuBoostMode if CPU <= 1963.5MHz or GPU >= 76.8MHz
                if (IsCpuBoostMode() && ((module == SysClkModule_CPU && hz <= CPU_BOOST_FREQ) || module == SysClkModule_GPU))
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
    this->oc->tickWaitTimeMs = this->GetConfig()->GetConfigValue(SysClkConfigValue_PollingIntervalMs);
    uint64_t tickWaitTimeNs = this->oc->tickWaitTimeMs * 1000000ULL;

    if (   FileUtils::IsBoostEnabled()
        && this->context->realProfile != SysClkProfile_Handheld
        && this->context->enabled
        && this->context->freqs[SysClkModule_CPU] <= CPU_BOOST_FREQ)
    {
        uint64_t systemCoreIdleTickPrev = 0, systemCoreIdleTickNext = 0;
        svcGetInfo(&systemCoreIdleTickPrev, InfoType_IdleTickCount, INVALID_HANDLE, 3);
        svcSleepThread(tickWaitTimeNs);
        svcGetInfo(&systemCoreIdleTickNext, InfoType_IdleTickCount, INVALID_HANDLE, 3);

        /* Convert idletick to free% */
        /* If CPU core usage is 0%, then idletick = 19'200'000 per sec */
        uint64_t systemCoreIdleTick = systemCoreIdleTickNext - systemCoreIdleTickPrev;
        uint64_t freeIdleTick = 19'200 * this->oc->tickWaitTimeMs;
        uint8_t  freePerc = systemCoreIdleTick / (freeIdleTick / 100);

        uint8_t systemCoreBoostFreeThreshold = 5;
        switch (this->context->realProfile)
        {
            case SysClkProfile_HandheldChargingOfficial:
            case SysClkProfile_Docked:
                systemCoreBoostFreeThreshold = 10;
                break;
            default:
                break;
        }

        bool systemCoreBoostCPU_PrevState = this->oc->systemCoreBoostCPU;
        this->oc->systemCoreBoostCPU = (freePerc <= systemCoreBoostFreeThreshold);

        if (systemCoreBoostCPU_PrevState && !this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, GetHz(SysClkModule_CPU));
        }
        else if (!systemCoreBoostCPU_PrevState && this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, CPU_BOOST_FREQ);
        }
    }
    else
    {
        svcSleepThread(tickWaitTimeNs);
    }

    /* Signal that systemCore is not busy */
    // this->oc->systemCoreStuckCount = 0;
}

/* Tricky part, see IpcService implementation of calling static member function */
// I don't see any use of this @ 1020 MHz (only useful at lowest freq),
// and downclock below 1020 MHz won't save battery. (same volt @ 600mV)
/*void ClockManager::CheckSystemCoreStuck(void *arg)
{
    ClockManager* clkMgr = (ClockManager*)arg;
    while (clkMgr->oc->systemCoreCheckStuck)
    {
        svcSleepThread(clkMgr->oc->tickWaitTimeMs * 1000000ULL);

        if (clkMgr->context->freqs[SysClkModule_CPU] >= clkMgr->CPU_BOOST_FREQ)
            continue;

        // Core #0,1,2 will check if Core#3 is stuck (threshold count = 2*3 = 6)
        // ! Add mutex !
        if (clkMgr->oc->systemCoreStuckCount++ > 6)
        {
            // Signal that current core will take over to boost CPU and wait some time to recheck
            clkMgr->oc->systemCoreStuckCount = -6;
            Clocks::SetHz(SysClkModule_CPU, clkMgr->CPU_BOOST_FREQ);
        }
    }
}*/

/*void ClockManager::StartCheckSystemCore()
{
    this->oc->systemCoreCheckStuck = true;
    threadCreate(&this->t_CheckSystemCoreStuck_0, this->CheckSystemCoreStuck, NULL, NULL, 0x1000, 0x20, 0);
    threadCreate(&this->t_CheckSystemCoreStuck_1, this->CheckSystemCoreStuck, NULL, NULL, 0x1000, 0x20, 1);
    threadCreate(&this->t_CheckSystemCoreStuck_2, this->CheckSystemCoreStuck, NULL, NULL, 0x1000, 0x20, 2);
    threadStart(&this->t_CheckSystemCoreStuck_0);
    threadStart(&this->t_CheckSystemCoreStuck_1);
    threadStart(&this->t_CheckSystemCoreStuck_2);
}*/

/*void ClockManager::StopCheckSystemCore()
{
    this->oc->systemCoreCheckStuck = false;
    threadWaitForExit(&this->t_CheckSystemCoreStuck_0);
    threadWaitForExit(&this->t_CheckSystemCoreStuck_1);
    threadWaitForExit(&this->t_CheckSystemCoreStuck_2);
    threadClose(&this->t_CheckSystemCoreStuck_0);
    threadClose(&this->t_CheckSystemCoreStuck_1);
    threadClose(&this->t_CheckSystemCoreStuck_2);
}*/

SysClkProfile ClockManager::ReverseNXProfileHandler()
{
    if (!IsReverseNXEnabled())
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

ReverseNXMode ClockManager::ReverseNXFileHandler(bool checkTool, const char* filePath)
{
    FILE *readFile;
    readFile = fopen(filePath, "rb");

    if (!readFile)
    {
        return ReverseNX_NotFound;
    }

    if (checkTool)
    {
        uint64_t magicDocked   = 0xD65F03C0320003E0;
        uint64_t magicHandheld = 0xD65F03C052A00000;

        uint64_t readBuffer = 0;
        fread(&readBuffer, 1, sizeof(readBuffer), readFile);
        fclose(readFile);

        if (!memcmp(&readBuffer, &magicDocked, sizeof(readBuffer)))
            return ReverseNX_Docked;

        if (!memcmp(&readBuffer, &magicHandheld, sizeof(readBuffer)))
            return ReverseNX_Handheld;
    }
    else // checkRT
    {
        uint64_t tidBuffer = 0;
        uint8_t ctrlBuffer = 0;
        fread(&tidBuffer, 1, sizeof(tidBuffer), readFile);
        fread(&ctrlBuffer, 1, sizeof(ctrlBuffer), readFile);
        fclose(readFile);
        remove(filePath);

        if (!memcmp(&this->context->applicationId, &tidBuffer, sizeof(tidBuffer)))
        {
            /* If user toggles -RT overlay, mode set by -Tool remains invalid until the game restarts */
            this->oc->reverseNXToolMode = ReverseNX_NotValid;

            ReverseNXMode getMode = static_cast<ReverseNXMode>(ctrlBuffer);
            this->oc->reverseNXRTMode = getMode;

            return getMode;
        }
    }

    return ReverseNX_NotValid;
}

void ClockManager::CheckReverseNXTool()
{
    bool shouldCheckReverseNXTool = FileUtils::IsReverseNXSyncEnabled() && FileUtils::ExistReverseNXTool();
    if (!shouldCheckReverseNXTool)
        return;

    ReverseNXMode getMode = ReverseNX_NotValid;
    if (this->context->applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
    {
        const char asmFileName[] = "_ZN2nn2oe18GetPerformanceModeEv.asm64"; // Checking one asm64 file is enough
        char asmFilePath[128];

        /* Check global override */
        snprintf(asmFilePath, sizeof(asmFilePath), "/SaltySD/patches/%s", asmFileName);
        getMode = ReverseNXFileHandler(true, asmFilePath);

        if (!getMode)
        {
            /* Check per-game override */
            snprintf(asmFilePath, sizeof(asmFilePath), "/SaltySD/patches/%016lX/%s", this->context->applicationId, asmFileName);
            getMode = ReverseNXFileHandler(true, asmFilePath);
        }
    }

    this->oc->reverseNXToolMode = getMode;
}

bool ClockManager::RefreshContext()
{
    bool hasChanged = false;
    bool shouldAdjustProfile = false;

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
        shouldAdjustProfile = true;

        /* Clear ReverseNX-RT state and recheck -Tool patches*/
        this->oc->reverseNXRTMode = ReverseNX_SystemDefault;
        CheckReverseNXTool();
    }

    SysClkProfile profile = Clocks::GetCurrentProfile();
    if (profile != this->context->realProfile)
    {
        FileUtils::LogLine("[mgr] Profile change: %s", Clocks::GetProfileName(profile, true));
        this->context->realProfile = profile;
        hasChanged = true;
        shouldAdjustProfile = true;
    }

    /* Check ReverseNX-RT once per tick */
    {
        ReverseNXMode getMode = ReverseNXFileHandler(false, FILE_REVERSENX_RT_CONF_PATH);
        if (getMode)
        {
            this->oc->reverseNXRTMode = getMode;
            shouldAdjustProfile = true;
        }

        if (getMode == ReverseNX_RTResetToDefault)
        {
            this->oc->reverseNXRTMode = ReverseNX_SystemDefault;
        }

        if (shouldAdjustProfile)
        {
            this->context->profile = ReverseNXProfileHandler();
        }
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
    if(hasChanged)
        Clocks::ResetToStock();

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
