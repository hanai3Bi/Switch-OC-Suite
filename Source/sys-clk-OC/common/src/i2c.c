#include <sysclk/i2c.h>

Result I2cSet_U8(I2cDevice dev, u8 reg, u8 val) {
    // ams::fatal::srv::StopSoundTask::StopSound()
    // I2C Bus Communication Reference: https://www.ti.com/lit/an/slva704/slva704.pdf
    struct {
        u8 reg;
        u8 val;
    } __attribute__((packed)) cmd;

    I2cSession _session;
    Result res = i2cOpenSession(&_session, dev);
    if (res)
        return res;

    cmd.reg = reg;
    cmd.val = val;
    res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
    i2csessionClose(&_session);
    return res;
}

Result I2cRead_OutU8(I2cDevice dev, u8 reg, u8 *out) {
    struct { u8 reg; } __attribute__((packed)) cmd;
    struct { u8 val; } __attribute__((packed)) rec;

    I2cSession _session;
    Result res = i2cOpenSession(&_session, dev);
    if (res)
        return res;

    cmd.reg = reg;
    res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
    if (res) {
        i2csessionClose(&_session);
        return res;
    }

    res = i2csessionReceiveAuto(&_session, &rec, sizeof(rec), I2cTransactionOption_All);
    i2csessionClose(&_session);
    if (res) {
        return res;
    }

    *out = rec.val;
    return 0;
}

Result I2cRead_OutU16(I2cDevice dev, u8 reg, u16 *out) {
    struct { u8 reg;  } __attribute__((packed)) cmd;
    struct { u16 val; } __attribute__((packed)) rec;

    I2cSession _session;
    Result res = i2cOpenSession(&_session, dev);
    if (res)
        return res;

    cmd.reg = reg;
    res = i2csessionSendAuto(&_session, &cmd, sizeof(cmd), I2cTransactionOption_All);
    if (res) {
        i2csessionClose(&_session);
        return res;
    }

    res = i2csessionReceiveAuto(&_session, &rec, sizeof(rec), I2cTransactionOption_All);
    i2csessionClose(&_session);
    if (res) {
        return res;
    }

    *out = rec.val;
    return 0;
}

float I2c_Max17050_GetBatteryCurrent() {
    u16 val;
    Result res = I2cRead_OutU16(I2cDevice_Max17050, MAX17050_CURRENT_REG, &val);
    if (res)
        return 0.f;

    const float SenseResistor   = 5.; // in uOhm
    const float CGain           = 1.99993;
    return (s16)val * (1.5625 / (SenseResistor * CGain));
}

u32 I2c_Max77812_GetMVOUT(I2c_Max77812_Volt_Type type) {
    u8 reg = (u8)type;

    const u32 MIN_MV    = 250;
    const u32 MV_STEP   = 5;
    const u8  RESET_VAL = 0x78;

    u8 val = RESET_VAL;
    // Retry 3 times if received RESET_VAL
    for (int i = 0; i < 3; i++) {
        if (I2cRead_OutU8(I2cDevice_Max77812_2, reg, &val))
            return 0u;
        if (val != RESET_VAL)
            break;
        svcSleepThread(10);
    }

    return val * MV_STEP + MIN_MV;
}

u8 I2c_Bq24193_Convert_mA_Raw(u32 ma) {
    // Adjustment is required
    u8 raw = 0;

    if (ma > MA_RANGE_MAX) // capping
        ma = MA_RANGE_MAX;

    bool pct20 = ma <= (MA_RANGE_MIN - 64);
    if (pct20) {
        ma = ma * 5;
        raw |= 0x1;
    }

    ma -= ma % 100; // round to 100
    ma -= (MA_RANGE_MIN - 64); // ceiling
    raw |= (ma >> 6) << 2;

    return raw;
};

u32 I2c_Bq24193_Convert_Raw_mA(u8 raw) {
    // No adjustment is allowed
    u32 ma = (((raw >> 2)) << 6) + MA_RANGE_MIN;

    bool pct20 = raw & 1;
    if (pct20)
        ma = ma * 20 / 100;

    return ma;
};

Result I2c_Bq24193_GetFastChargeCurrentLimit(u32 *ma) {
    u8 raw;
    Result res = I2cRead_OutU8(I2cDevice_Bq24193, BQ24193_CHARGE_CURRENT_CONTROL_REG, &raw);
    if (res)
        return res;

    *ma = I2c_Bq24193_Convert_Raw_mA(raw);
    return 0;
}

Result I2c_Bq24193_SetFastChargeCurrentLimit(u32 ma) {
    u8 raw = I2c_Bq24193_Convert_mA_Raw(ma);
    return I2cSet_U8(I2cDevice_Bq24193, BQ24193_CHARGE_CURRENT_CONTROL_REG, raw);
}