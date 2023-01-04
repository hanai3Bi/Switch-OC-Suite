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

class StepTrackBarIcon : public tsl::elm::StepTrackBar {
public:
    StepTrackBarIcon(const char icon[3], size_t numSteps):
        tsl::elm::StepTrackBar(icon, numSteps) { }
    const char* getIcon() { return this->m_icon; }
    void setIcon(const char* icon) { this->m_icon = icon; }
};

class MiscGui : public BaseMenuGui
{
    public:
        MiscGui();
        ~MiscGui();
        void listUI() override;
        void refresh() override;

    protected:
        typedef struct {
            float batCurrent;
            u32 cpuVolt  = 620;
            u32 gpuVolt  = 610;
            u32 dramVolt = 600;
        } I2cInfo;

        void PsmUpdate(uint32_t dispatchId = 0)
        {
            psmInitialize();

            if (dispatchId)
            {
                serviceDispatch(psmGetServiceSession(), dispatchId);
                svcSleepThread(1000);
            }

            serviceDispatchOut(psmGetServiceSession(), 17, *(this->chargeInfo));
            psmGetBatteryChargePercentage(&(this->batteryChargePerc));

            psmExit();
        }

        void I2cGetInfo(I2cInfo* i2cInfo)
        {
            i2cInitialize();

            i2cInfo->batCurrent = I2c_Max17050_GetBatteryCurrent();

            i2cInfo->cpuVolt  = I2c_Max77812_GetMVOUT(I2c_Max77812_CPUVOLT_REG);
            i2cInfo->gpuVolt  = I2c_Max77812_GetMVOUT(I2c_Max77812_GPUVOLT_REG);
            i2cInfo->dramVolt = I2c_Max77812_GetMVOUT(I2c_Max77812_MEMVOLT_REG);

            I2c_Bq24193_GetFastChargeCurrentLimit(reinterpret_cast<u32 *>(&(chargeInfo->ChargeCurrentLimit)));

            i2cExit();
        }

        void UpdateInfo(char* out, size_t outsize)
        {
            float chargerVoltLimit = (float)chargeInfo->ChargerVoltageLimit / 1000;
            float chargerCurrLimit = (float)chargeInfo->ChargerCurrentLimit / 1000;
            float chargerOutWatts  = chargerVoltLimit * chargerCurrLimit;

            char chargeVoltLimit[20] = "";
            if (chargeInfo->ChargeVoltageLimit)
                snprintf(chargeVoltLimit, sizeof(chargeVoltLimit), ", %umV", chargeInfo->ChargeVoltageLimit);

            char chargWattsInfo[50] = "";
            if (chargeInfo->ChargerType)
                snprintf(chargWattsInfo, sizeof(chargWattsInfo), " %.1fV/%.1fA (%.1fW)", chargerVoltLimit, chargerCurrLimit, chargerOutWatts);

            char batCurInfo[30] = "";
            if (std::abs(i2cInfo->batCurrent) < 10)
                snprintf(batCurInfo, sizeof(batCurInfo), "0mA");
            else
                snprintf(batCurInfo, sizeof(batCurInfo), "%+.2fmA (%+.2fW)",
                    i2cInfo->batCurrent, i2cInfo->batCurrent * (float)chargeInfo->VoltageAvg / 1000'000);

            snprintf(out, outsize,
                "%s%s\n"
                "%.3fV %.2f\u00B0C\n"
                "+%umA, -%umA\n"
                "+%umA%s\n"
                "%.2f%%\n"
                "%.2f%%\n"
                "%s\n"
                "%s\n\n"
                "%dmV\n"
                "%dmV\n"
                "%dmV\n"
                ,
                PsmInfoChargerTypeToStr(chargeInfo->ChargerType), chargWattsInfo,
                (float)chargeInfo->VoltageAvg / 1000, (float)chargeInfo->BatteryTemperature / 1000,
                chargeInfo->InputCurrentLimit, chargeInfo->VBUSCurrentLimit,
                chargeInfo->ChargeCurrentLimit, chargeVoltLimit,
                (float)chargeInfo->RawBatteryCharge / 1000,
                (float)chargeInfo->BatteryAge / 1000,
                PsmPowerRoleToStr(chargeInfo->PowerRole),
                batCurInfo,
                i2cInfo->cpuVolt,
                i2cInfo->gpuVolt,
                i2cInfo->dramVolt
            );
        }

        void PsmChargingToggler(bool* enable)
        {
            if (!PsmIsChargerConnected(this->chargeInfo))
            {
                *enable = false;
                return;
            }

            PsmUpdate(*enable ? 2 : 3);

            *enable = (PsmIsCharging(this->chargeInfo) == *enable);
        }

        void LblUpdate(bool shouldSwitch = false)
        {
            lblInitialize();

            lblGetBacklightSwitchStatus(&lblstatus);
            if (shouldSwitch)
                lblstatus ? lblSwitchBacklightOff(0) : lblSwitchBacklightOn(0);

            lblExit();
        }

        bool isMariko = false;

        std::map<SysClkConfigValue, tsl::elm::ToggleListItem*> configToggles;
        void addConfigToggle(SysClkConfigValue);
        void updateConfigToggles();

        tsl::elm::ToggleListItem *chargingDisabledOverrideToggle, *backlightToggle;
        tsl::elm::CategoryHeader *chargingCurrentHeader, *chargingLimitHeader;
        StepTrackBarIcon *chargingCurrentBar, *chargingLimitBar;

        SysClkConfigValueList* configList;
        PsmChargeInfo*  chargeInfo;
        I2cInfo*        i2cInfo;
        LblBacklightSwitchStatus lblstatus = LblBacklightSwitchStatus_Disabled;

        const char* infoNames = "Charger:\nBattery:\nCurrent Limit:\nCharging Limit:\nRaw Charge:\nBattery Age:\nPower Role:\nCurrent Flow:\n\nCPU Volt:\nGPU Volt:\nDRAM Volt:";
        char infoVals[300] = "";
        char chargingLimitBarDesc[40] = "";
        char chargingCurrentBarDesc[45] = "";
        u32 batteryChargePerc = 0;
        u8 frameCounter = 60;
};
