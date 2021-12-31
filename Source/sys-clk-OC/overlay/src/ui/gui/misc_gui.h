/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#pragma once

#include "../../ipc.h"
#include "base_menu_gui.h"
#include <inttypes.h>

class MiscGui : public BaseMenuGui
{
    public:
        MiscGui();
        ~MiscGui();
        void preDraw(tsl::gfx::Renderer* render) override;
        void listUI() override;
        void update() override;

    protected:
        typedef enum {
            PDCtrler_NewPDO       = 1, //Received new Power Data Object
            PDCtrler_NoPD         = 2, //No Power Delivery source is detected
            PDCtrler_AcceptedRDO  = 3  //Received and accepted Request Data Object
        } ChargeInfoPDCtrler; //BM92T series

        typedef enum {
            PowerRole_Sink        = 1,
            PowerRole_Source      = 2
        } ChargeInfoPowerRole;

        constexpr const char* ChargeInfoPowerRoleToStr(ChargeInfoPowerRole in)
        {
            switch (in)
            {
                case PowerRole_Sink:    return "Sink";
                case PowerRole_Source:  return "Source";
                default:                return "Unknown";
            }
        };

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

        constexpr const char* ChargeInfoChargerTypeToStr(ChargeInfoChargerType in) noexcept
        {
            switch (in)
            {
                case ChargerType_None:          return "None";
                case ChargerType_PD:            return "USB-C PD";
                case ChargerType_TypeC_1500mA:
                case ChargerType_TypeC_3000mA:  return "USB-C";
                case ChargerType_DCP:           return "USB DCP";
                case ChargerType_CDP:           return "USB CDP";
                case ChargerType_SDP:           return "USB SDP";
                case ChargerType_Apple_500mA:
                case ChargerType_Apple_1000mA:
                case ChargerType_Apple_2000mA:  return "Apple";
                default:                        return "Unknown";
            }
        };

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

        typedef enum
        {
            Max17050Reg_Current     = 0x0A,
            Max17050Reg_AvgCurrent  = 0x0B,
            Max17050Reg_Cycle       = 0x17,
        } Max17050Reg;

        void PsmUpdate(uint32_t dispatchId = 0)
        {
            smInitialize();
            psmInitialize();

            if (dispatchId)
            {
                serviceDispatch(psmGetServiceSession(), dispatchId);
                svcSleepThread(1000);
            }

            serviceDispatchOut(psmGetServiceSession(), 17, *(this->chargeInfo));
            psmIsEnoughPowerSupplied(&(this->isEnoughPowerSupplied));

            psmExit();
            smExit();
        }

        bool PsmIsChargerConnected()
        {
            return this->chargeInfo->ChargerType != ChargerType_None;
        }

        bool PsmIsCharging()
        {
            return PsmIsChargerConnected() && ((this->chargeInfo->unk_x14 >> 8) & 1);
        }

        bool PsmIsFastCharging()
        {
            return this->chargeInfo->ChargeCurrentLimit > 768;
        }

        bool PsmIsEnoughPowerSupplied()
        {
            return this->isEnoughPowerSupplied;
        }

        Result I2cReadRegHandler(u8 reg, I2cDevice dev, u16 *out)
        {
            // ams::fatal::srv::StopSoundTask::StopSound()
            // I2C Bus Communication Reference: https://www.ti.com/lit/an/slva704/slva704.pdf
            struct { u8 reg;  } __attribute__((packed)) cmd;
            struct { u16 val; } __attribute__((packed)) rec;

            I2cSession _session;

            Result res = i2cOpenSession(&_session, dev);
            if (res)
                return res;

            cmd.reg = reg;
            res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
            if (res)
            {
                i2csessionClose(&_session);
                return res;
            }

            res = i2csessionReceiveAuto(&_session, &rec, sizeof(rec), I2cTransactionOption_All);
            if (res)
            {
                i2csessionClose(&_session);
                return res;
            }

            *out = rec.val;
            i2csessionClose(&_session);
            return 0;
        }

        bool Max17050ReadReg(u8 reg, u16 *out)
        {
            u16 data = 0;
            Result res = I2cReadRegHandler(reg, I2cDevice_Max17050, &data);

            if (res)
            {
                *out = res;
                return false;
            }

            *out = data;
            return true;
        }

        void GetInfo(char* out, size_t outsize)
        {
            float chargerVoltLimit = (float)chargeInfo->ChargerVoltageLimit / 1000;
            float chargerCurrLimit = (float)chargeInfo->ChargerCurrentLimit / 1000;
            float chargerOutWatts  = chargerVoltLimit * chargerCurrLimit;

            float batCurrent = 0;
            float batCycleCount = 0;

            char chargeVoltLimit[20] = "";
            if (chargeInfo->ChargeVoltageLimit)
                snprintf(chargeVoltLimit, sizeof(chargeVoltLimit), ", %u mV", chargeInfo->ChargeVoltageLimit);

            // From Hekate, too lazy to query configuration from reg
            constexpr float max17050SenseResistor = 5.; // in uOhm
            constexpr float max17050CGain = 1.99993;

            // Init, read registers and exit I2C service all here to save resources
            {
                smInitialize();
                i2cInitialize();
                u16 data = 0;

                if (Max17050ReadReg(Max17050Reg_Cycle, &data))
                    batCycleCount = data / 100.;

                if (Max17050ReadReg(Max17050Reg_Current, &data))
                    batCurrent = (s16)data * (1.5625 / (max17050SenseResistor * max17050CGain));

                i2cExit();
                smExit();
            }

            char batWattsInfo[20] = "";
            if (std::abs(batCurrent) > 100)
                snprintf(batWattsInfo, sizeof(batWattsInfo), " (%+.2f W)", batCurrent * (float)chargeInfo->VoltageAvg / 1000'000);

            snprintf(out, outsize,
                "%s"
                "\nCharger:             %s %.1fV/%.1fA (%.1fW)"
                "\nBattery:               %.3fV %.2f\u00B0C"
                "\nCurrent Limit:     %u mA IN, %u mA OUT"
                "\nCharging Limit:  %u mA%s"
                "\nRaw Charge:     %.2f%%"
                "\nBattery Age:      %.2f%%"
                "\nPower Role:       %s"
                "\nCycle Count:     %.2f (Reset at power-up)"
                "\nCurrent Flow:    %+.2f mA%s"
                ,
                PsmIsEnoughPowerSupplied() ? "Enough Power Supplied" : "",
                ChargeInfoChargerTypeToStr(chargeInfo->ChargerType), chargerVoltLimit, chargerCurrLimit, chargerOutWatts,
                (float)chargeInfo->VoltageAvg / 1000,
                (float)chargeInfo->BatteryTemperature / 1000,
                chargeInfo->InputCurrentLimit, chargeInfo->VBUSCurrentLimit,
                chargeInfo->ChargeCurrentLimit, chargeVoltLimit,
                (float)chargeInfo->RawBatteryCharge / 1000,
                (float)chargeInfo->BatteryAge / 1000,
                ChargeInfoPowerRoleToStr(chargeInfo->PowerRole),
                batCycleCount,
                batCurrent, batWattsInfo
            );
        }

        bool PsmChargingToggler(bool enable)
        {
            if (!PsmIsChargerConnected())
                return false;

            PsmUpdate(enable ? 2 : 3);

            return PsmIsCharging() == enable;
        }

        bool PsmFastChargingToggler(bool enable)
        {
            if (!PsmIsChargerConnected())
                return false;

            PsmUpdate(enable ? 10 : 11);

            return PsmIsFastCharging() == enable;
        }

        tsl::elm::ToggleListItem *chargingToggle, *fastChargingToggle;

        ChargeInfo* chargeInfo;
        bool isEnoughPowerSupplied = false;
        char infoOutput[800] = "";
        int frameCounter = 60;
};
