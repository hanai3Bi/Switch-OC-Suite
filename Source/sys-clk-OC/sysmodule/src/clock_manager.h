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

    static ClockManager* GetInstance();
    static void Initialize();
    static void Exit();

    void SetRunning(bool running);
    bool Running();
    void Tick();
    void WaitForNextTick();
    SysClkContext GetCurrentContext();
    Config* GetConfig();

    typedef enum {
        PDCtrler_NewPDO       = 1, //Received new Power Data Object
        PDCtrler_NoPD         = 2, //No Power Delivery source is detected
        PDCtrler_AcceptedRDO  = 3  //Received and accepted Request Data Object
    } ChargeInfoPDCtrler; //BM92T series

    typedef enum {
        PowerRole_Sink        = 1,
        PowerRole_Source      = 2
    } ChargeInfoPowerRole;

    typedef enum {
        ChargerType_None         = 0,
        ChargerType_PD           = 1,
        ChargerType_TypeC_1500mA = 2,
        ChargerType_TypeC_3000mA = 3,
        ChargerType_DCP          = 4,
        ChargerType_CDP          = 5,
        ChargerType_SDP          = 6,
        ChargerType_Apple_500mA  = 7,
        ChargerType_Apple_1000mA = 8,
        ChargerType_Apple_2000mA = 9
    } ChargeInfoChargerType;

    typedef enum {
        Flags_NoHub       = BIT(0),  //If hub is disconnected
        Flags_Rail        = BIT(8),  //At least one Joy-con is charging from rail
        Flags_SPDSRC      = BIT(12), //OTG
        Flags_ACC         = BIT(16)  //Accessory
    } ChargeInfoFlags;

    typedef struct {
        int32_t InputCurrentLimit;          //Input (Sink) current limit in mA
        int32_t VBUSCurrentLimit;           //Output (Source/VBUS/OTG) current limit in mA
        int32_t ChargeCurrentLimit;         //Battery charging current limit in mA (512mA when Docked, 768mA when BatteryTemperature < 17.0 C)
        int32_t ChargeVoltageLimit;         //Battery charging voltage limit in mV (3952mV when BatteryTemperature >= 51.0 C)
        int32_t unk_x10;                    //Possibly an emum, getting the same value as PowerRole in all tested cases
        int32_t unk_x14;                    //Possibly flags
        ChargeInfoPDCtrler PDCtrlerState;   //Power Delivery Controller State
        int32_t BatteryTemperature;         //Battery temperature in milli C
        int32_t RawBatteryCharge;           //Raw battery charged capacity per cent-mille (i.e. 100% = 100000 pcm)
        int32_t VoltageAvg;                 //Voltage avg in mV (more in Notes)
        int32_t BatteryAge;                 //Battery age (capacity full / capacity design) per cent-mille (i.e. 100% = 100000 pcm)
        ChargeInfoPowerRole PowerRole;
        ChargeInfoChargerType ChargerType;
        int32_t ChargerVoltageLimit;        //Charger and external device voltage limit in mV
        int32_t ChargerCurrentLimit;        //Charger and external device current limit in mA
        ChargeInfoFlags Flags;              //Unknown flags
    } ChargeInfo;

    typedef enum {
      EnableBatteryCharging      = 2,
      DisableBatteryCharging     = 3,
      EnableFastBatteryCharging  = 10,
      DisableFastBatteryCharging = 11,
      GetBatteryChargeInfoFields = 17,
    } IPsmServerCmd;

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

    SysClkOcExtra *oc;

    bool IsCpuBoostMode();
    bool IsReverseNXModeValid();
    bool IsReverseNXDocked();

    uint32_t GetHz(SysClkModule);
    SysClkProfile ReverseNXProfileHandler();
    ReverseNXMode ReverseNXFileHandler(const char*);

    void CheckReverseNXTool();
    bool CheckReverseNXRT();
    void ChargingHandler();

};
