/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#pragma once

#include <atomic>
#include <sysclk.h>

#include "config.h"
#include "clocks.h"
#include <nxExt/cpp/lockable_mutex.h>

class ClockManager
{
  public:
    const std::uint32_t MAX_CPU = 1963500000;

    static ClockManager* GetInstance();
    static void Initialize();
    static void Exit();

    bool recheckReverseNX = false;
    bool isEnabledReverseNX = false;
    bool isEnabledReverseNXTool = false;
    bool isDockedReverseNX = false;
    std::uint16_t tickCheckReverseNXRT = 0;
    std::uint16_t tickStartBoost = 0;
    char prevReverseNXRT = 0;
    SysClkProfile RealProfile;

    bool IsCpuBoostMode();
    SysClkProfile ReverseNXProfile(bool);
    void checkReverseNXTool();
    bool GameStartBoost();

    void SetRunning(bool running);
    bool Running();
    void Tick();
    void WaitForNextTick();
    void checkReverseNXToolAsm(FILE*, uint8_t*);
    void checkReverseNXRT(bool, uint8_t*);
    SysClkContext GetCurrentContext();
    Config* GetConfig();

  protected:
    ClockManager();
    virtual ~ClockManager();

    bool RefreshContext();

    static ClockManager *instance;
    std::atomic_bool running;
    LockableMutex contextMutex;
    Config *config;
    SysClkContext *context;
    std::uint64_t lastTempLogNs;
    std::uint64_t lastCsvWriteNs;
};
