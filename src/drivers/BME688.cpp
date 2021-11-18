#include "BME688.h"
#include <Logger.h>

BME688::BME688(){
    //
}

u8 BME688::init(){

    u8 data[4];

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__CHIP_ID, data, 1);
    u8 chip_id = data[0];

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_T1_LSB, data, 2);
    t1 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_T2_LSB, data, 2);
    t2 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_T3, data, 1);
    t3 = data[0];

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P1_LSB, data, 2);
    p1 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P2_LSB, data, 2);
    p2 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P3, data, 1);
    p3 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P4_LSB, data, 2);
    p4 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P5_LSB, data, 2);
    p5 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P6, data, 1);
    p6 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P7, data, 1);
    p7 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P8_LSB, data, 2);
    p8 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P9_LSB, data, 2);
    p9 = (data[1] << 8 ) | data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_P10, data, 1);
    p10 = data[0];

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H2_MSB, data, 3);
    h1 = (data[0] << 4 ) | (data[1] & 0xF);
    h2 = (data[2] << 4 ) | (data[1] >> 4 );

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H3, data, 1);
    h3 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H4, data, 1);
    h4 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H5, data, 1);
    h5 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H6, data, 1);
    h6 = data[0];
    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PAR_H7, data, 1);
    h7 = data[0];

    return chip_id;

}

void BME688::start_measurment(){

        u8 data[2];
        data[0] = BME688_REG__CTRL_MEAS;
        data[1] = 0x25;
        FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);

}

ErrorType BME688::read_sensors(i16 &calc_temp, i32 &pressure_comp, i32 &calc_hum){

    u8 rxData[8];

    const auto err2 = FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__MEAS_STATUS_0, rxData, 1);
    if(err2 != ErrorType::SUCCESS){
        return err2;
    }

    logt("XENA", "Measurment Status: %d", rxData[0]);

    const auto err = FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PRESS_MSB_0, rxData, 8);
    if(err != ErrorType::SUCCESS){
        return err;
    }

    u32 temp_adc = (rxData[3] << 12 ) | (rxData[4] << 4 ) | (rxData[5] >> 4);

    i32 t_fine;
    {

        int64_t var1;
        int64_t var2;
        int64_t var3;

        /*lint -save -e701 -e702 -e704 */
        var1 = ((int32_t)temp_adc >> 3) - ((int32_t)t1 << 1);
        var2 = (var1 * (int32_t)t2) >> 11;
        var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
        var3 = ((var3) * ((int32_t)t3 << 4)) >> 14;
        t_fine = (int32_t)(var2 + var3);
        calc_temp = (int16_t)(((t_fine * 5) + 128) >> 8);

    }

    u32 pres_adc = (rxData[0] << 12 ) | (rxData[1] << 4 ) | (rxData[2] >> 4);

    {

        int32_t var1;
        int32_t var2;
        int32_t var3;

        /* This value is used to check precedence to multiplication or division
        * in the pressure compensation equation to achieve least loss of precision and
        * avoiding overflows.
        * i.e Comparing value, pres_ovf_check = (1 << 31) >> 1
        */
        const int32_t pres_ovf_check = INT32_C(0x40000000);

        /*lint -save -e701 -e702 -e713 */
        var1 = (((int32_t)t_fine) >> 1) - 64000;
        var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)p6) >> 2;
        var2 = var2 + ((var1 * (int32_t)p5) << 1);
        var2 = (var2 >> 2) + ((int32_t)p4 << 16);
        var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)p3 << 5)) >> 3) +
            (((int32_t)p2 * var1) >> 1);
        var1 = var1 >> 18;
        var1 = ((32768 + var1) * (int32_t)p1) >> 15;
        pressure_comp = 1048576 - pres_adc;
        pressure_comp = (int32_t)((pressure_comp - (var2 >> 12)) * ((uint32_t)3125));
        if (pressure_comp >= pres_ovf_check)
        {
            pressure_comp = ((pressure_comp / var1) << 1);
        }
        else
        {
            pressure_comp = ((pressure_comp << 1) / var1);
        }

        var1 = ((int32_t)p9 * (int32_t)(((pressure_comp >> 3) * (pressure_comp >> 3)) >> 13)) >> 12;
        var2 = ((int32_t)(pressure_comp >> 2) * (int32_t)p8) >> 13;
        var3 =
            ((int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) *
            (int32_t)p10) >> 17;
        pressure_comp = (int32_t)(pressure_comp) + ((var1 + var2 + var3 + ((int32_t)p7 << 7)) >> 4);

    }

    u16 hum_adc = (rxData[6] << 8 ) | rxData[7];

    {
        int32_t var1;
        int32_t var2;
        int32_t var3;
        int32_t var4;
        int32_t var5;
        int32_t var6;
        int32_t temp_scaled;

        /*lint -save -e702 -e704 */
        temp_scaled = (((int32_t)t_fine * 5) + 128) >> 8;
        var1 = (int32_t)(hum_adc - ((int32_t)((int32_t)h1 * 16))) -
            (((temp_scaled * (int32_t)h3) / ((int32_t)100)) >> 1);
        var2 =
            ((int32_t)h2 *
            (((temp_scaled * (int32_t)h4) / ((int32_t)100)) +
            (((temp_scaled * ((temp_scaled * (int32_t)h5) / ((int32_t)100))) >> 6) / ((int32_t)100)) +
            (int32_t)(1 << 14))) >> 10;
        var3 = var1 * var2;
        var4 = (int32_t)h6 << 7;
        var4 = ((var4) + ((temp_scaled * (int32_t)h7) / ((int32_t)100))) >> 4;
        var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
        var6 = (var4 * var5) >> 1;
        calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
        if (calc_hum > 100000) /* Cap at 100%rH */
        {
            calc_hum = 100000;
        }
        else if (calc_hum < 0)
        {
            calc_hum = 0;
        }

    }

    return ErrorType::SUCCESS;

}