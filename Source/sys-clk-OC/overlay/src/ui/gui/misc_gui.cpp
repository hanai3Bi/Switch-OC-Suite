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
    this->listElement->addItem(new tsl::elm::CategoryHeader("Config"));

    this->unsafeFreqToggle = addConfigToggle(SysClkConfigValue_AllowUnsafeFrequencies, "Allow Unsafe Frequencies");
    this->cpuBoostToggle = addConfigToggle(SysClkConfigValue_AutoCPUBoost, "Auto CPU Boost");
    this->syncModeToggle = addConfigToggle(SysClkConfigValue_SyncReverseNXMode, "Sync ReverseNX Mode");
    this->fastChargingToggle = addConfigToggle(SysClkConfigValue_DisableFastCharging, "Disable Fast Charging");

    this->chargingLimitHeader = new tsl::elm::CategoryHeader("");
    this->listElement->addItem(this->chargingLimitHeader);
    this->chargingLimitBar = new StepTrackBarIcon("", 100 + 1);
    this->chargingLimitBar->setProgress(this->configList->values[SysClkConfigValue_ChargingLimitPercentage]);
    this->chargingLimitBar->setValueChangedListener([this](u8 val) {
        if (val < 20) {
            val = 20;
            this->chargingLimitBar->setProgress(val);
        }
        this->configList->values[SysClkConfigValue_ChargingLimitPercentage] = val;

        snprintf(chargingLimitBarDesc, 30, "Battery Charging Limit: %lu%%", this->configList->values[SysClkConfigValue_ChargingLimitPercentage]);
        this->chargingLimitHeader->setText(chargingLimitBarDesc);
        this->chargingLimitBar->setIcon(getBatteryStateIcon());

        Result rc = sysclkIpcSetConfigValues(this->configList);
        if (R_FAILED(rc))
            FatalGui::openWithResultCode("sysclkIpcSetConfigValues", rc);

        this->lastContextUpdate = armGetSystemTick();
    });
    this->listElement->addItem(this->chargingLimitBar);
    this->listElement->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
      renderer->drawString("\uE016 Long-term use may render the battery gauge \ninaccurate!", false, x, y + 20, SMALL_TEXT_SIZE, DESC_COLOR);
    }), SMALL_TEXT_SIZE * 2 + 20);

    this->listElement->addItem(new tsl::elm::CategoryHeader("Temporary toggles"));

    this->backlightToggle = new tsl::elm::ToggleListItem("Screen Backlight", false);
    backlightToggle->setStateChangedListener([this](bool state) {
        LblUpdate(true);
    });
    this->listElement->addItem(this->backlightToggle);

    this->listElement->addItem(new tsl::elm::CategoryHeader("Info"));
    this->listElement->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(this->infoNames, false, x, y + 20, SMALL_TEXT_SIZE, DESC_COLOR);
        renderer->drawString(this->infoVals, false, x + 120, y + 20, SMALL_TEXT_SIZE, VALUE_COLOR);
    }), SMALL_TEXT_SIZE * 12 + 20);
}

void MiscGui::refresh() {
    BaseMenuGui::refresh();

    if (this->context && ++frameCounter >= 60)
    {
        frameCounter = 0;
        sysclkIpcGetConfigValues(this->configList);
        updateConfigToggle(this->unsafeFreqToggle, SysClkConfigValue_AllowUnsafeFrequencies);
        updateConfigToggle(this->cpuBoostToggle, SysClkConfigValue_AutoCPUBoost);
        updateConfigToggle(this->syncModeToggle, SysClkConfigValue_SyncReverseNXMode);
        updateConfigToggle(this->fastChargingToggle, SysClkConfigValue_DisableFastCharging);

        this->chargingLimitBar->setProgress(this->configList->values[SysClkConfigValue_ChargingLimitPercentage]);
        snprintf(chargingLimitBarDesc, 30, "Battery Charging Limit: %lu%%", this->configList->values[SysClkConfigValue_ChargingLimitPercentage]);
        this->chargingLimitHeader->setText(chargingLimitBarDesc);

        PsmUpdate();
        this->chargingLimitBar->setIcon(getBatteryStateIcon());

        LblUpdate();
        this->backlightToggle->setState(lblstatus);

        I2cGetInfo(this->i2cInfo);
        UpdateInfo(this->infoVals, sizeof(this->infoVals));
    }
}
