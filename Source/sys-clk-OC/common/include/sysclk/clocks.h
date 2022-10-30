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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    SysClkProfile_Handheld = 0,
    SysClkProfile_HandheldCharging,
    SysClkProfile_HandheldChargingUSB,
    SysClkProfile_HandheldChargingOfficial,
    SysClkProfile_Docked,
    SysClkProfile_EnumMax
} SysClkProfile;

typedef enum
{
    SysClkModule_CPU = 0,
    SysClkModule_GPU,
    SysClkModule_MEM,
    SysClkModule_EnumMax
} SysClkModule;

typedef enum
{
    SysClkThermalSensor_SOC = 0,
    SysClkThermalSensor_PCB,
    SysClkThermalSensor_Skin,
    SysClkThermalSensor_EnumMax
} SysClkThermalSensor;

typedef struct
{
    uint8_t  enabled;
    uint64_t applicationId;
    SysClkProfile profile;
    uint32_t freqs[SysClkModule_EnumMax];
    uint32_t overrideFreqs[SysClkModule_EnumMax];
    uint32_t temps[SysClkThermalSensor_EnumMax];
    uint32_t perfConfId;
} SysClkContext;

typedef enum
{
    ReverseNX_NotFound = 0,
    ReverseNX_SystemDefault = 0,
    ReverseNX_Handheld,
    ReverseNX_Docked,
} ReverseNXMode;

typedef struct
{
    bool systemCoreBoostCPU;
    bool allowUnsafeFreq;
    bool governor;
    SysClkProfile realProfile;
} SysClkOcExtra;

typedef struct
{
    uint32_t values[20];
} SysClkFrequencyTable;

uint32_t* GetTable(SysClkModule module);

typedef struct
{
    union {
        uint32_t mhz[(size_t)SysClkProfile_EnumMax * (size_t)SysClkModule_EnumMax];
        uint32_t mhzMap[SysClkProfile_EnumMax][SysClkModule_EnumMax];
    };
    bool governorDisabled;
} SysClkTitleProfileList;

#define SYSCLK_GLOBAL_PROFILE_TID       0xA111111111111111

extern uint32_t g_freq_table_mem_hz[];
extern uint32_t g_freq_table_cpu_hz[];
extern uint32_t g_freq_table_gpu_hz[];

#define SYSCLK_ENUM_VALID(n, v) ((v) < n##_EnumMax)

static inline const char* sysclkFormatModule(SysClkModule module, bool pretty)
{
    switch(module)
    {
        case SysClkModule_CPU:
            return pretty ? "CPU" : "cpu";
        case SysClkModule_GPU:
            return pretty ? "GPU" : "gpu";
        case SysClkModule_MEM:
            return pretty ? "Memory" : "mem";
        default:
            return NULL;
    }
}

static inline const char* sysclkFormatThermalSensor(SysClkThermalSensor thermSensor, bool pretty)
{
    switch(thermSensor)
    {
        case SysClkThermalSensor_SOC:
            return pretty ? "SOC" : "soc";
        case SysClkThermalSensor_PCB:
            return pretty ? "PCB" : "pcb";
        case SysClkThermalSensor_Skin:
            return pretty ? "Skin" : "skin";
        default:
            return NULL;
    }
}

static inline const char* sysclkFormatProfile(SysClkProfile profile, bool pretty)
{
    switch(profile)
    {
        case SysClkProfile_Docked:
            return pretty ? "Docked" : "docked";
        case SysClkProfile_Handheld:
            return pretty ? "Handheld" : "handheld";
        case SysClkProfile_HandheldCharging:
            return pretty ? "Charging" : "handheld_charging";
        case SysClkProfile_HandheldChargingUSB:
            return pretty ? "USB Charger" : "handheld_charging_usb";
        case SysClkProfile_HandheldChargingOfficial:
            return pretty ? "Official Charger" : "handheld_charging_official";
        default:
            return NULL;
    }
}
