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
            smInitialize();
            psmInitialize();

            if (dispatchId)
            {
                serviceDispatch(psmGetServiceSession(), dispatchId);
                svcSleepThread(1000);
            }

            serviceDispatchOut(psmGetServiceSession(), 17, *(this->chargeInfo));

            psmExit();
            smExit();
        }

        Result I2cRead_OutU16(u8 reg, I2cDevice dev, u16 *out)
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

        Result I2cRead_OutU8(u8 reg, I2cDevice dev, u8 *out)
        {
            struct { u8 reg; } __attribute__((packed)) cmd;
            struct { u8 val; } __attribute__((packed)) rec;

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

        void I2cGetInfo(I2cInfo* i2cInfo)
        {
            smInitialize();
            i2cInitialize();

            // Max17050 fuel gauge regs. Sense resistor and CGain are from Hekate, too lazy to query configuration from reg
            constexpr float SenseResistor       = 5.; // in uOhm
            constexpr float CGain               = 1.99993;
            constexpr u8 Max17050Reg_Current    = 0x0A;
            // constexpr u8 Max17050Reg_AvgCurrent = 0x0B;
            // constexpr u8 Max17050Reg_Cycle      = 0x17;
            u16 tmp = 0;
            if (R_SUCCEEDED(I2cRead_OutU16(Max17050Reg_Current, I2cDevice_Max17050, &tmp)))
                i2cInfo->batCurrent = (s16)tmp * (1.5625 / (SenseResistor * CGain));

            auto I2cRead_Max77812_M_VOUT = [this](u8 reg)
            {
                constexpr u32 MIN_MV    = 250;
                constexpr u32 MV_STEP   = 5;
                constexpr u8  RESET_VAL = 0x78;

                u8 tmp = RESET_VAL;
                // Retry 3 times if received RESET_VAL
                for (int i = 0; i < 3; i++) {
                    if (R_FAILED(I2cRead_OutU8(reg, I2cDevice_Max77812_2, &tmp)))
                        return 0u;
                    if (tmp != RESET_VAL)
                        break;
                    usleep(10);
                }

                return tmp * MV_STEP + MIN_MV;
            };

            constexpr u8 Max77812Reg_CPUVolt  = 0x26;
            constexpr u8 Max77812Reg_GPUVolt  = 0x23;
            constexpr u8 Max77812Reg_DRAMVolt = 0x25;
            i2cInfo->cpuVolt  = I2cRead_Max77812_M_VOUT(Max77812Reg_CPUVolt);
            i2cInfo->gpuVolt  = I2cRead_Max77812_M_VOUT(Max77812Reg_GPUVolt);
            i2cInfo->dramVolt = I2cRead_Max77812_M_VOUT(Max77812Reg_DRAMVolt);

            i2cExit();
            smExit();
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
                (float)chargeInfo->VoltageAvg / 1000,
                (float)chargeInfo->BatteryTemperature / 1000,
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
            smInitialize();
            lblInitialize();
            lblGetBacklightSwitchStatus(&lblstatus);
            if (shouldSwitch)
                lblstatus ? lblSwitchBacklightOff(0) : lblSwitchBacklightOn(0);
            lblExit();
            smExit();
        }

        tsl::elm::ToggleListItem* addConfigToggle(SysClkConfigValue);
        void updateConfigToggle(tsl::elm::ToggleListItem*, SysClkConfigValue);

        tsl::elm::ToggleListItem *backlightToggle, *unsafeFreqToggle, *cpuBoostToggle, *syncModeToggle, *fastChargingToggle, *governorToggle;
        tsl::elm::CategoryHeader *chargingLimitHeader;
        StepTrackBarIcon *chargingLimitBar;

        SysClkConfigValueList* configList;
        PsmChargeInfo*  chargeInfo;
        I2cInfo*        i2cInfo;
        LblBacklightSwitchStatus lblstatus = LblBacklightSwitchStatus_Disabled;

        const char* infoNames = "Charger:\nBattery:\nCurrent Limit:\nCharging Limit:\nRaw Charge:\nBattery Age:\nPower Role:\nCurrent Flow:\n\nCPU Volt:\nGPU Volt:\nDRAM Volt:";
        char infoVals[300] = "";
        char chargingLimitBarDesc[30] = "";
        u8 frameCounter = 60;
};
