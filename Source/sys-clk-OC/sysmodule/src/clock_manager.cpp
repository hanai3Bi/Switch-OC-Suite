/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#define FORCE_ALL_HANDHELD_MODES_TO_USE_DOCK_CLOCK
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
    this->oc->tickWaitTimeMs = 500;
    this->oc->systemCoreBoostThreshold = 100;
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
    if(confId == 0x92220009 || confId == 0x9222000A)
        return true;
    else
        return false;
}

SysClkProfile ClockManager::ReverseNXProfile(bool ForceDock)
{
    RealProfile = Clocks::GetCurrentProfile();
    switch(RealProfile)
    {
        case SysClkProfile_HandheldChargingOfficial:
#ifdef FORCE_ALL_HANDHELD_MODES_TO_USE_DOCK_CLOCK
        case SysClkProfile_HandheldChargingUSB:
        case SysClkProfile_HandheldCharging:
        case SysClkProfile_Handheld:
#endif
            if (ForceDock)
                return SysClkProfile_Docked;
            else
                return RealProfile;
        case SysClkProfile_Docked:
            if (ForceDock)
                return SysClkProfile_Docked;
            else
                return FileUtils::IsDownclockDockEnabled() ? SysClkProfile_HandheldChargingOfficial : SysClkProfile_Docked;
        default:
            return RealProfile;
    }
}

void ClockManager::checkReverseNXTool()
{
    char ReverseNXToolAsm[] = "_ZN2nn2oe18GetPerformanceModeEv.asm64"; // Checking one asm64 file is enough
    char ReverseNXToolAsmPath[128];
    uint8_t flag = 0;
    snprintf(ReverseNXToolAsmPath, sizeof ReverseNXToolAsmPath, "/SaltySD/patches/%s", ReverseNXToolAsm);

    FILE *readReverseNXToolAsm;
    readReverseNXToolAsm = fopen(ReverseNXToolAsmPath, "rb");

    // Enforce mode globally: Enabled
    if(readReverseNXToolAsm != NULL)
    {
        checkReverseNXToolAsm(readReverseNXToolAsm, &flag);
        switch(flag)
        {
            case 1:
                FileUtils::LogLine("[mgr] ReverseNX-Tool patches detected: Enforce Handheld globally");
                this->context->profile = ReverseNXProfile(false);
                isDockedReverseNX = false;
                isEnabledReverseNX = true;
                isEnabledReverseNXTool = true;
                break;
            case 2:
                FileUtils::LogLine("[mgr] ReverseNX-Tool patches detected: Enforce Docked globally");
                this->context->profile = ReverseNXProfile(true);
                isDockedReverseNX = true;
                isEnabledReverseNX = true;
                isEnabledReverseNXTool = true;
                break;
        }
    }
    else
    {
        snprintf(ReverseNXToolAsmPath, sizeof ReverseNXToolAsmPath, "/SaltySD/patches/%016lX/%s", this->context->applicationId, ReverseNXToolAsm);
        readReverseNXToolAsm = fopen(ReverseNXToolAsmPath, "rb");
        // Found game-specific setting
        if(readReverseNXToolAsm != NULL)
        {
            checkReverseNXToolAsm(readReverseNXToolAsm, &flag);
            switch(flag)
            {
                case 1:
                    FileUtils::LogLine("[mgr] ReverseNX-Tool patches detected: Force Handheld in %016lX", this->context->applicationId);
                    this->context->profile = ReverseNXProfile(false);
                    isDockedReverseNX = false;
                    isEnabledReverseNX = true;
                    isEnabledReverseNXTool = true;
                    break;
                case 2:
                    FileUtils::LogLine("[mgr] ReverseNX-Tool patches detected: Force Docked in %016lX", this->context->applicationId);
                    this->context->profile = ReverseNXProfile(true);
                    isDockedReverseNX = true;
                    isEnabledReverseNX = true;
                    isEnabledReverseNXTool = true;
                    break;
                default:
                    isEnabledReverseNXTool = false;
            }
        }
    }
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

    /* Per-Game setting */
    if (!hz)
        hz = this->config->GetAutoClockHz(this->context->applicationId, module, this->context->profile);

    /* Global setting */
    if (!hz)
        hz = this->config->GetAutoClockHz(0xA111111111111111, module, this->context->profile);

    /* Adjust hz if ReverseNX is enabled */
    if (!hz && isEnabledReverseNX)
    {
        switch(module)
        {
            case SysClkModule_CPU:
                hz = 1020'000'000;
                break;
            case SysClkModule_GPU:
                if (!isDockedReverseNX && ((FileUtils::IsDownclockDockEnabled() && RealProfile == SysClkProfile_Docked)
                                        || RealProfile != SysClkProfile_Docked))
                    hz = 460'800'000;
                else
                    hz = 768'000'000;
                break;
            case SysClkModule_MEM:
                if (!isDockedReverseNX && ((FileUtils::IsDownclockDockEnabled() && RealProfile == SysClkProfile_Docked)
                                        || RealProfile != SysClkProfile_Docked))
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
        hz = Clocks::GetNearestHz(module, isEnabledReverseNX ? RealProfile : this->context->profile, hz);

        /* Ignore if MEM Max Hz > 1600 MHz */
        if (module == SysClkModule_MEM && hz == 1600'000'000 && this->context->freqs[module] >= hz)
        {
            return 0;
        }
    }

    if (module == SysClkModule_CPU)
    {
        if (this->oc->systemCoreBoostCPU && hz < MAX_CPU)
            return MAX_CPU;
        else if (!hz)
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
                if (IsCpuBoostMode())
                {
                    // Skip setting CPU or GPU clocks in CpuBoostMode if CPU <= 1963.5MHz or GPU >= 76.8MHz
                    if ((module == SysClkModule_CPU && hz <= MAX_CPU) || module == SysClkModule_GPU)
                    {
                        continue;
                    }
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

    if (   this->context->enabled
        && this->context->profile != SysClkProfile_Handheld
        && this->context->freqs[SysClkModule_CPU] <= MAX_CPU)
    {
        uint64_t systemCoreIdleTickPrev = 0, systemCoreIdleTickNext = 0;
        svcGetInfo(&systemCoreIdleTickPrev, InfoType_IdleTickCount, INVALID_HANDLE, 3);
        svcSleepThread(this->oc->tickWaitTimeMs * 1000000ULL);
        svcGetInfo(&systemCoreIdleTickNext, InfoType_IdleTickCount, INVALID_HANDLE, 3);

        /* Convert idletick to load% */
        /* If CPU core usage is 0%, then idletick = 19'200'000 per sec */
        uint64_t systemCoreIdleTick = systemCoreIdleTickNext - systemCoreIdleTickPrev;
        uint64_t freeIdleTick = 19'200 * this->oc->tickWaitTimeMs;
        uint8_t  freePerc = systemCoreIdleTick / (freeIdleTick / 100);

        bool systemCoreBoostCPU_PrevState = this->oc->systemCoreBoostCPU;

        switch (this->context->profile)
        {
            case SysClkProfile_HandheldChargingOfficial:
            case SysClkProfile_Docked:
                this->oc->systemCoreBoostThreshold = systemCoreBoostCPU_PrevState ? 6 : 10;
                break;
            default: // Unofficial charger
                this->oc->systemCoreBoostThreshold = systemCoreBoostCPU_PrevState ? 3 : 5;
                break;
        }

        this->oc->systemCoreBoostCPU = (freePerc <= this->oc->systemCoreBoostThreshold);

        if (systemCoreBoostCPU_PrevState && !this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, GetHz(SysClkModule_CPU));
        }
        else if (!systemCoreBoostCPU_PrevState && this->oc->systemCoreBoostCPU)
        {
            Clocks::SetHz(SysClkModule_CPU, MAX_CPU);
        }
    }
    else
    {
        svcSleepThread(this->oc->tickWaitTimeMs * 1000000ULL);
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

        if (clkMgr->context->freqs[SysClkModule_CPU] >= clkMgr->MAX_CPU)
            continue;

        // Core #0,1,2 will check if Core#3 is stuck (threshold count = 2*3 = 6)
        // ! Add mutex !
        if (clkMgr->oc->systemCoreStuckCount++ > 6)
        {
            // Signal that current core will take over to boost CPU and wait some time to recheck
            clkMgr->oc->systemCoreStuckCount = -6;
            Clocks::SetHz(SysClkModule_CPU, clkMgr->MAX_CPU);
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

void ClockManager::checkReverseNXToolAsm(FILE* readFile, uint8_t* flag)
{
    // Magic Value
    uint8_t Docked[0x10] = {0xE0, 0x03, 0x00, 0x32, 0xC0, 0x03, 0x5F, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t Handheld[0x10] = {0x00, 0x00, 0xA0, 0x52, 0xC0, 0x03, 0x5F, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t filebuffer[0x10] = {0};
    fread(&filebuffer, 1, 16, readFile);

    uint8_t cmpresult = 0;
    cmpresult = memcmp(filebuffer, Docked, sizeof(Docked));
    if (cmpresult != 0)
    {
        cmpresult = memcmp(filebuffer, Handheld, sizeof(Handheld));
        if (cmpresult != 0)
            *flag = 0; // Set to default
        else
            *flag = 1; // Handheld
    }
    else
        *flag = 2; // Docked

    fclose(readFile);
}

void ClockManager::checkReverseNXRT(bool recheckReverseNX, uint8_t* flag)
{
    FILE* readReverseNXRTConf = fopen(FILE_REVERSENX_RT_CONF_PATH, "rb");
    if (readReverseNXRTConf != NULL)
    {
        uint8_t ReverseNXRTConfArr[9];
        fread(ReverseNXRTConfArr, 9, 1, readReverseNXRTConf);
        fclose(readReverseNXRTConf);
        remove(FILE_REVERSENX_RT_CONF_PATH);

        uint8_t currentTid[8];
        for(int i = 0; i < 8; i++)
            currentTid[i] = this->context->applicationId >> 8*(7-i);

        uint8_t cmpresult = memcmp(currentTid, ReverseNXRTConfArr, sizeof(currentTid));

        if (cmpresult == 0)
            *flag = ReverseNXRTConfArr[8]; // 1: Handheld, 2: Docked, 3: Reset
        else
            *flag = 0;  // 0: Not applicable
    }
    else if (recheckReverseNX)
        *flag = prevReverseNXRT; // Use previous state when profile changes
    else
        *flag = 0;
}

bool ClockManager::RefreshContext()
{
    bool hasChanged = false;

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
        prevReverseNXRT = 0; // Reset ReverseNX-RT previous state when Title ID changes
        this->context->applicationId = applicationId;
        hasChanged = true;

        if (FileUtils::IsReverseNXSyncEnabled() && (FileUtils::IsReverseNXToolExist() || recheckReverseNX))
        {
            // A new game starts or the real profile changes, then we need to check if ReverseNXTool patches are applied
            isEnabledReverseNX = false;

            // Check if ReverseNXTool patches are applied
            if (applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
                this->checkReverseNXTool();
        }
    }

    if (FileUtils::IsReverseNXSyncEnabled() && (!tickCheckReverseNXRT || recheckReverseNX))
    {
        uint8_t flag = 0;
        checkReverseNXRT(recheckReverseNX, &flag);

        switch(flag)
        {
            case 1:
                FileUtils::LogLine("[mgr] ReverseNX-RT detected: Enforce Handheld Mode");
                this->context->profile = ReverseNXProfile(false);
                prevReverseNXRT = flag;
                isEnabledReverseNX = true;
                isDockedReverseNX = false;
                hasChanged = true;
                break;
            case 2:
                FileUtils::LogLine("[mgr] ReverseNX-RT detected: Enforce Docked Mode");
                this->context->profile = ReverseNXProfile(true);
                prevReverseNXRT = flag;
                isEnabledReverseNX = true;
                isDockedReverseNX = true;
                hasChanged = true;
                break;
            case 3:
                FileUtils::LogLine("[mgr] ReverseNX-RT disabled: Reset to System-controlled Mode and recheck ReverseNX-Tool");
                RealProfile = Clocks::GetCurrentProfile();
                this->context->profile = RealProfile;
                prevReverseNXRT = 0;
                isEnabledReverseNX = false;
                isDockedReverseNX = false;
                hasChanged = true;
                if (this->context->applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
                    this->checkReverseNXTool();
                break;
            case 0:
                if (recheckReverseNX && isEnabledReverseNXTool && this->context->applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
                    this->checkReverseNXTool();
                break;
        }
        tickCheckReverseNXRT = (std::uint32_t)( 1'000 / this->oc->tickWaitTimeMs ) + 1;
    }
    tickCheckReverseNXRT--;

    if (recheckReverseNX)
        recheckReverseNX = false;

    SysClkProfile profile = Clocks::GetCurrentProfile();
    if (profile != this->context->profile && !isEnabledReverseNX)
    {
        FileUtils::LogLine("[mgr] Profile change: %s", Clocks::GetProfileName(profile, true));
        this->context->profile = profile;
        hasChanged = true;
    }
    if (profile != RealProfile && isEnabledReverseNX)
    {
        FileUtils::LogLine("[mgr] Profile change: %s, recheck ReverseNX", Clocks::GetProfileName(profile, true));
        this->context->profile = profile;
        RealProfile = profile;
        hasChanged = true;
        recheckReverseNX = true;
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
