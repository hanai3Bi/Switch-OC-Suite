#pragma once

#include <switch.h>

// To use i2c service, sm and i2c should be intialized via smInitialize() and i2cInitialize().

Result I2cSet_U8(I2cDevice dev, u8 reg, u8 val);

Result I2cRead_OutU8(I2cDevice dev, u8 reg, u8 *out);
Result I2cRead_OutU16(I2cDevice dev, u8 reg, u16 *out);

// Max17050 fuel gauge
float I2c_Max17050_GetBatteryCurrent();

const u8 MAX17050_CURRENT_REG = 0x0A;

// Max77812 Mariko-specific buck converter
typedef enum I2c_Max77812_Volt_Type {
    I2c_Max77812_CPUVOLT_REG = 0x26,
    I2c_Max77812_GPUVOLT_REG = 0x23,
    I2c_Max77812_MEMVOLT_REG = 0x25,
} I2c_Max77812_Volt_Type;

u32 I2c_Max77812_GetMVOUT(I2c_Max77812_Volt_Type type);

// Bq24193 Battery management
u32 I2c_Bq24193_Convert_Raw_mA(u8 raw);
u8  I2c_Bq24193_Convert_mA_Raw(u32 ma);

Result I2c_Bq24193_GetFastChargeCurrentLimit(u32 *ma);
Result I2c_Bq24193_SetFastChargeCurrentLimit(u32 ma);

const u32 MA_RANGE_MIN = 512;
const u32 MA_RANGE_MAX = 4544;

const u8 BQ24193_CHARGE_CURRENT_CONTROL_REG = 0x2;