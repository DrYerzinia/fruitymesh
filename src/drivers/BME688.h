#pragma once

#include "FruityHal.h"

#define BME688_ADDR                 0x76

#define BME688_REG__MEAS_STATUS_0   0x1D

#define BME688_REG__PRESS_MSB_0     0x1F
#define BME688_REG__PRESS_LSB_0     0x20
#define BME688_REG__PRESS_XLSB_0    0x21

#define BME688_REG__TEMP_MSB_0      0x22
#define BME688_REG__TEMP_LSB_0      0x23
#define BME688_REG__TEMP_XLSB_0     0x24

#define BME688_REG__HUM_MSB_0       0x25
#define BME688_REG__HUM_LSB_0       0x26

#define BME688_REG__CTRL_GAS_0      0x70
#define BME688_REG__CTRL_GAS_1      0x71
#define BME688_REG__CTRL_HUM        0x72
#define BME688_REG__CTRL_MEAS       0x74

#define BME688_REG__PAR_P1_LSB       0x8E
#define BME688_REG__PAR_P1_MSB       0x8F
#define BME688_REG__PAR_P2_LSB       0x90
#define BME688_REG__PAR_P2_MSB       0x91
#define BME688_REG__PAR_P3           0x92
#define BME688_REG__PAR_P4_LSB       0x94
#define BME688_REG__PAR_P4_MSB       0x95
#define BME688_REG__PAR_P5_LSB       0x96
#define BME688_REG__PAR_P5_MSB       0x97
#define BME688_REG__PAR_P6           0x99
#define BME688_REG__PAR_P7           0x98
#define BME688_REG__PAR_P8_LSB       0x9C
#define BME688_REG__PAR_P8_MSB       0x9D
#define BME688_REG__PAR_P9_LSB       0x9E
#define BME688_REG__PAR_P9_MSB       0x9F
#define BME688_REG__PAR_P10          0xA0

#define BME688_REG__PAR_H1_LSB       0xE2
#define BME688_REG__PAR_H1_MSB       0xE3
#define BME688_REG__PAR_H2_LSB       0xE2
#define BME688_REG__PAR_H2_MSB       0xE1
#define BME688_REG__PAR_H3           0xE4
#define BME688_REG__PAR_H4           0xE5
#define BME688_REG__PAR_H5           0xE6
#define BME688_REG__PAR_H6           0xE7
#define BME688_REG__PAR_H7           0xE8

#define BME688_REG__PAR_T1_LSB       0xE9
#define BME688_REG__PAR_T1_MSB       0xEA
#define BME688_REG__PAR_T2_LSB       0x8A
#define BME688_REG__PAR_T2_MSB       0x8B
#define BME688_REG__PAR_T3           0x8C

#define BME688_REG__CHIP_ID         0xD0 // 0x61

class BME688
{
public:

    BME688();

    u8 init();

    void start_measurment();
    ErrorType read_sensors(i16 &calc_temp, i32 &pressure_comp, i32 &calc_hum);

private:

    u16 t1;
    i16 t2;
    i8  t3;

    u16 p1;
    i16 p2;
    i8  p3;
    i16 p4;
    i16 p5;
    i8  p6;
    i8  p7;
    i16 p8;
    i16 p9;
    u8  p10;

    u16 h1;
    u16 h2;
    i8  h3;
    i8  h4;
    i8  h5;
    u8  h6;
    i8  h7;

};