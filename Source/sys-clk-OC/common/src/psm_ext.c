#include <sysclk/psm_ext.h>

const char* PsmPowerRoleToStr(PsmPowerRole role) {
    switch (role) {
        case PsmPowerRole_Sink:     return "Sink";
        case PsmPowerRole_Source:   return "Source";
        default:                    return "Unknown";
    }
}

const char* PsmInfoChargerTypeToStr(PsmInfoChargerType type) {
    switch (type) {
        case PsmInfoChargerType_None:           return "None";
        case PsmInfoChargerType_PD:             return "USB-C PD";
        case PsmInfoChargerType_TypeC_1500mA:
        case PsmInfoChargerType_TypeC_3000mA:   return "USB-C";
        case PsmInfoChargerType_DCP:            return "USB DCP";
        case PsmInfoChargerType_CDP:            return "USB CDP";
        case PsmInfoChargerType_SDP:            return "USB SDP";
        case PsmInfoChargerType_Apple_500mA:
        case PsmInfoChargerType_Apple_1000mA:
        case PsmInfoChargerType_Apple_2000mA:   return "Apple";
        default:                                return "Unknown";
    }
}

bool PsmIsChargerConnected(const PsmChargeInfo* info) {
    return info->ChargerType != PsmInfoChargerType_None;
}

bool PsmIsCharging(const PsmChargeInfo* info) {
    return PsmIsChargerConnected(info) && ((info->unk_x14 >> 8) & 1);
}

bool PsmIsFastChargingEnabled(const PsmChargeInfo* info) {
    return info->ChargeCurrentLimit > 768;
}

PsmBatteryState PsmGetBatteryState(const PsmChargeInfo* info) {
    if (!PsmIsChargerConnected(info))
        return PsmBatteryState_Discharging;
    if (!PsmIsCharging(info))
        return PsmBatteryState_ChargingPaused;
    return PsmIsFastChargingEnabled(info) ? PsmBatteryState_FastCharging : PsmBatteryState_SlowCharging;
}

const char* PsmGetBatteryStateIcon(const PsmChargeInfo* info) {
    switch (PsmGetBatteryState(info)) {
        case PsmBatteryState_Discharging:   return "\u25c0"; // ◀
        case PsmBatteryState_ChargingPaused:return "| |";
        case PsmBatteryState_SlowCharging:  return "\u25b6"; // ▶
        case PsmBatteryState_FastCharging:  return "\u25b6\u25b6"; // ▶▶
        default:                            return "?";
    }
}
