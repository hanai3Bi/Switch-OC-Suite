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
#include <cstdint>
#include <switch.h>
#include <sysclk.h>

#define MAX_MEM_CLOCK 1862'400'000
#define BOOST_THRESHOLD 95'0

class Clocks
{
  public:
    static inline uint32_t boostCpuFreq = 1785000000;
    static inline uint32_t maxMemFreq = 0;

    static void GetRange(SysClkModule module, SysClkProfile profile, uint32_t** min, uint32_t** max);
    static Result GetTable(SysClkModule module, SysClkProfile profile, SysClkFrequencyTable* out_table);
    static void SetAllowUnsafe(bool allow) { allowUnsafe = allow; };
    static bool GetIsMariko() { return isMariko; };
    static void Exit();
    static void Initialize();
    static SysClkApmConfiguration* GetEmbeddedApmConfig(uint32_t confId);
    static uint32_t GetStockClock(SysClkApmConfiguration* apm, SysClkModule module);
    static void ResetToStock(unsigned int module = SysClkModule_EnumMax);
    static SysClkProfile GetCurrentProfile();
    static std::uint32_t GetCurrentHz(SysClkModule module);
    static void SetHz(SysClkModule module, std::uint32_t hz);
    static const char* GetProfileName(SysClkProfile profile, bool pretty);
    static const char* GetModuleName(SysClkModule module, bool pretty);
    static const char* GetThermalSensorName(SysClkThermalSensor sensor, bool pretty);
    static std::uint32_t GetNearestHz(SysClkModule module, SysClkProfile profile, std::uint32_t inHz);
    static std::uint32_t GetTemperatureMilli(SysClkThermalSensor sensor);

  protected:
    static inline bool allowUnsafe;
    static inline bool isMariko;
    static std::int32_t GetTsTemperatureMilli(TsLocation location);
    static PcvModule GetPcvModule(SysClkModule sysclkModule);
    static PcvModuleId GetPcvModuleId(SysClkModule sysclkModule);
    static std::uint32_t GetMaxAllowedHz(SysClkModule module, SysClkProfile profile);
};
