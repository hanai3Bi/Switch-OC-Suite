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
    this->running = false;
    this->lastTempLogNs = 0;
    this->lastCsvWriteNs = 0;
}

ClockManager::~ClockManager()
{
    delete this->config;
    delete this->context;
}

bool ClockManager::IsCpuBoostMode()
{
    std::uint32_t confId = 0;
    Result rc = 0;
    rc = apmExtGetCurrentPerformanceConfiguration(&confId);
    ASSERT_RESULT_OK(rc, "apmExtGetCurrentPerformanceConfiguration");
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

bool ClockManager::GameStartBoost()
{
    if (tickStartBoost && this->GetConfig()->Enabled())
    {
        if (Clocks::GetCurrentHz(SysClkModule_CPU) != MAX_CPU)
        {
            Clocks::SetHz(SysClkModule_CPU, MAX_CPU);
            this->context->freqs[SysClkModule_CPU] = MAX_CPU;
        }

        std::uint64_t applicationId = ProcessManagement::GetCurrentApplicationId();
        // If user exit the game
        if (applicationId != this->context->applicationId)
        {
            tickStartBoost = 0;
            return false;
        }

        if (tickStartBoost == 1)
        {
            FileUtils::LogLine("[mgr] Boost done, reset to stock");
            Clocks::ResetToStock();
        }
        tickStartBoost--;
        return true;
    }

    return false;
}

void ClockManager::SetRunning(bool running)
{
    this->running = running;
}

bool ClockManager::Running()
{
    return this->running;
}

void ClockManager::Tick()
{
    std::scoped_lock lock{this->contextMutex};

    if(!GameStartBoost())
    {
        bool cpuBoost = FileUtils::IsBoostEnabled() ? IsCpuBoostMode() : false;
        if (this->RefreshContext() || this->config->Refresh())
        {
            std::uint32_t hz = 0;
            std::uint32_t hzForceOverride = 0;
            for (unsigned int module = 0; module < SysClkModule_EnumMax - 1; module++)
            {
                hz = this->context->overrideFreqs[module];

                if(!hz)
                {
                    hz = this->config->GetAutoClockHz(this->context->applicationId, (SysClkModule)module, this->context->profile);
                    hzForceOverride = this->config->GetAutoClockHz(0xA111111111111111, (SysClkModule)module, this->context->profile);
                    if (!hz && hzForceOverride)
                        hz = hzForceOverride;

                    if(isEnabledReverseNX && !hz)
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
                        }
                    }

                }

                if (hz)
                {
                    hz = Clocks::GetNearestHz((SysClkModule)module, isEnabledReverseNX ? RealProfile : this->context->profile, hz);

                    if (hz != this->context->freqs[module] && this->context->enabled)
                    {
                        if (cpuBoost)
                        {
                            // Skip setting CPU or GPU clocks in CpuBoostMode if CPU < 1963.5MHz or GPU > 76.8MHz
                            if (module == SysClkModule_CPU && hz < MAX_CPU)
                            {
                                continue;
                            }
                            if (module == SysClkModule_GPU && hz > 76'800'000)
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
    }
}

void ClockManager::WaitForNextTick()
{
    svcSleepThread(this->GetConfig()->GetConfigValue(SysClkConfigValue_PollingIntervalMs) * 1000000ULL);
}

void ClockManager::checkReverseNXToolAsm(FILE* readFile, uint8_t* flag)
{
    // Copied from ReverseNXTool
    uint8_t Docked[0x10] = {0xE0, 0x03, 0x00, 0x32, 0xC0, 0x03, 0x5F, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t Handheld[0x10] = {0x00, 0x00, 0xA0, 0x52, 0xC0, 0x03, 0x5F, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t filebuffer[0x10] = {0};
    uint8_t cmpresult = 0;
    fread(&filebuffer, 1, 16, readFile);
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

        if (FileUtils::IsBoostStartEnabled() && this->context->applicationId != PROCESS_MANAGEMENT_QLAUNCH_TID)
        {
            // If a game starts and overrides for CPU are not enabled, then set MAX_CPU for 10 sec
            std::uint32_t overcpu = this->context->overrideFreqs[SysClkModule_CPU];
            if (!overcpu)
            {
                tickStartBoost = (std::uint32_t)( 10'000 / this->GetConfig()->GetConfigValue(SysClkConfigValue_PollingIntervalMs) ) + 1;
                FileUtils::LogLine("[mgr] A game starts, bump CPU to max for 10 sec");
                return true;
            }
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
        // Check once per sec
        tickCheckReverseNXRT = (std::uint32_t)( 1'000 / this->GetConfig()->GetConfigValue(SysClkConfigValue_PollingIntervalMs) ) + 1;
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

    // restore clocks to stock values on app or profile change
    if(hasChanged)
        Clocks::ResetToStock();

    std::uint32_t hz = 0;
    for (unsigned int module = 0; module < SysClkModule_EnumMax; module++)
    {
        hz = Clocks::GetCurrentHz((SysClkModule)module);

        // Skip MEM freq check
        if (module == SysClkModule_MEM)
        {
            this->context->freqs[module] = hz;
            break;
        }

        // Round to MHz
        uint32_t cur_mhz = hz/1000'000;
        uint32_t be4_mhz = this->context->freqs[module]/1000'000;
        if (hz != 0 && cur_mhz != be4_mhz)
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
