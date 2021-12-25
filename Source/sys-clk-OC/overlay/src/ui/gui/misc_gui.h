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

        void PsmGetInfo(char* out, size_t outsize)
        {
            float chargerVoltLimit = (float)chargeInfo->ChargerVoltageLimit / 1000;
            float chargerCurrLimit = (float)chargeInfo->ChargerCurrentLimit / 1000;
            float chargerOutWatts  = chargerVoltLimit * chargerCurrLimit;

            char chargeVoltLimit[20] = "";
            if (chargeInfo->ChargeVoltageLimit)
                snprintf(chargeVoltLimit, sizeof(chargeVoltLimit), ", %u mV", chargeInfo->ChargeVoltageLimit);

            snprintf(out, outsize,
                "Charger:            %s %.1fV/%.1fA (%.1fW)"
                "\n%s"
                "\nBattery:              %.3fV %.2f\u00B0C"
                "\nCurrent Limit:    %u mA IN, %u mA OUT"
                "\nCharging Limit: %u mA%s"
                "\nRaw Charge:    %.2f%%"
                "\nBattery Age:     %.2f%%"
                "\nPower Role:      %s"
                ,
                ChargeInfoChargerTypeToStr(chargeInfo->ChargerType), chargerVoltLimit, chargerCurrLimit, chargerOutWatts,
                PsmIsEnoughPowerSupplied() ? "Enough Power Supplied" : "",
                (float)chargeInfo->VoltageAvg / 1000,
                (float)chargeInfo->BatteryTemperature / 1000,
                chargeInfo->InputCurrentLimit, chargeInfo->VBUSCurrentLimit,
                chargeInfo->ChargeCurrentLimit, chargeVoltLimit,
                (float)chargeInfo->RawBatteryCharge / 1000,
                (float)chargeInfo->BatteryAge / 1000,
                ChargeInfoPowerRoleToStr(chargeInfo->PowerRole)
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
        char psmOutput[800] = "";
        int frameCounter = 60;
};
