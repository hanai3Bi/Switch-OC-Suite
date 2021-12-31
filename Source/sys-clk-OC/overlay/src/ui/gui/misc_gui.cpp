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
    this->chargeInfo = new ChargeInfo;
}

MiscGui::~MiscGui()
{
    delete this->chargeInfo;
}

void MiscGui::preDraw(tsl::gfx::Renderer* render)
{
    BaseMenuGui::preDraw(render);

    render->drawString(this->infoOutput, false, 40, 300, SMALL_TEXT_SIZE, DESC_COLOR);
}

void MiscGui::listUI()
{
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
}

void MiscGui::update()
{
    BaseMenuGui::update();

    if (++frameCounter >= 60)
    {
        frameCounter = 0;
        PsmUpdate();
        GetInfo(this->infoOutput, sizeof(this->infoOutput));
        this->chargingToggle->setState(this->PsmIsCharging());
        this->fastChargingToggle->setState(this->PsmIsFastCharging());
    }
}
