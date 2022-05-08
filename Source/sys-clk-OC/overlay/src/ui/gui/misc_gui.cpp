/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "misc_gui.h"

#include "fatal_gui.h"
#include "../format.h"

MiscGui::MiscGui()
{
    this->configList = new SysClkConfigValueList {};
    this->chargeInfo = new ChargeInfo {};
    this->i2cInfo    = new I2cInfo {};
}

MiscGui::~MiscGui()
{
    delete this->configList;
    delete this->chargeInfo;
    delete this->i2cInfo;
}

tsl::elm::ToggleListItem* MiscGui::addConfigToggle(SysClkConfigValue configVal, std::string labelName) {
    tsl::elm::ToggleListItem* toggle = new tsl::elm::ToggleListItem(labelName, this->configList->values[configVal]);
    toggle->setStateChangedListener([this, configVal](bool state) {
        this->configList->values[configVal] = uint64_t(state);
        Result rc = sysclkIpcSetConfigValues(this->configList);
        if (R_FAILED(rc))
            FatalGui::openWithResultCode("sysclkIpcSetConfigValues", rc);

        this->lastContextUpdate = armGetSystemTick();
    });
    this->listElement->addItem(toggle);
    return toggle;
}

void MiscGui::updateConfigToggle(tsl::elm::ToggleListItem *toggle, SysClkConfigValue configVal) {
    if (toggle != nullptr) {
        toggle->setState(this->configList->values[configVal]);
    }
}

void MiscGui::listUI()
{
    sysclkIpcGetConfigValues(this->configList);

    this->cpuBoostToggle = addConfigToggle(SysClkConfigValue_AutoCPUBoost, "Auto CPU Boost");
    this->syncModeToggle = addConfigToggle(SysClkConfigValue_SyncReverseNXMode, "Sync ReverseNX Mode");

    // Charging
    this->chargingToggle = new tsl::elm::ToggleListItem("Charging", false);
    chargingToggle->setStateChangedListener([this](bool state) {
        if (PsmChargingToggler(state))
        {
            this->chargingToggle->setState(state);
            this->fastChargingToggle->setState(this->PsmIsFastCharging());
        }
        else
        {
            this->chargingToggle->setState(!state);
        }
    });
    this->listElement->addItem(this->chargingToggle);

    // FastCharging
    this->fastChargingToggle = new tsl::elm::ToggleListItem("Fast Charging", false);
    fastChargingToggle->setStateChangedListener([this](bool state) {
        if (PsmFastChargingToggler(state))
        {
            this->fastChargingToggle->setState(state);
        }
        else
        {
            this->fastChargingToggle->setState(!state);
        }
    });
    this->listElement->addItem(this->fastChargingToggle);

    // Backlight
    this->backlightToggle = new tsl::elm::ToggleListItem("Screen Backlight", false);
    backlightToggle->setStateChangedListener([this](bool state) {
        LblUpdate(true);
    });
    this->listElement->addItem(this->backlightToggle);

    this->listElement->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(this->infoOutput, false, x, y, SMALL_TEXT_SIZE, DESC_COLOR);
    }), SMALL_TEXT_SIZE * 13);
}

void MiscGui::refresh() {
    BaseMenuGui::refresh();

    if (this->context) {
        sysclkIpcGetConfigValues(this->configList);
        updateConfigToggle(this->cpuBoostToggle, SysClkConfigValue_AutoCPUBoost);
        updateConfigToggle(this->syncModeToggle, SysClkConfigValue_SyncReverseNXMode);
    }

    if (++frameCounter >= 60)
    {
        frameCounter = 0;
        PsmUpdate();
        LblUpdate();
        this->backlightToggle->setState(lblstatus);
        I2cGetInfo(this->i2cInfo);
        PrintInfo(this->infoOutput, sizeof(this->infoOutput));
        this->chargingToggle->setState(this->PsmIsCharging());
        this->fastChargingToggle->setState(this->PsmIsFastCharging());
    }
}
