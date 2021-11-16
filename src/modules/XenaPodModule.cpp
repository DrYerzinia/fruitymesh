////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2021 M-Way Solutions GmbH
// ** Contact: https://www.blureange.io/licensing
// **
// ** This file is part of the Bluerange/FruityMesh implementation
// **
// ** $BR_BEGIN_LICENSE:GPL-EXCEPT$
// ** Commercial License Usage
// ** Licensees holding valid commercial Bluerange licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between them and M-Way Solutions GmbH.
// ** For licensing terms and conditions see https://www.bluerange.io/terms-conditions. For further
// ** information use the contact form at https://www.bluerange.io/contact.
// **
// ** GNU General Public License Usage
// ** Alternatively, this file may be used under the terms of the GNU
// ** General Public License version 3 as published by the Free Software
// ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
// ** included in the packaging of this file. Please review the following
// ** information to ensure the GNU General Public License requirements will
// ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
// **
// ** $BR_END_LICENSE$
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////

#include "FruityHal.h"
#include <XenaPodModule.h>
#include <StatusReporterModule.h>
#include <GlobalState.h>
#include <Logger.h>
#include <Utility.h>
#include <IoModule.h>

#include <limits>

#include <BME688.h>

#if IS_ACTIVE(XENA_POD_MODULE)

namespace
{

    /// The interval between sensor measurements (and mesh broadcasts).
    constexpr u16 DEFAULT_SENSOR_MEASUREMENT_INTERVAL_DS = 100; //10;

}

XenaPodModule::XenaPodModule()
    : Module(XENA_POD_MODULE_ID, "xena_pod")
{

    //Enable the logtag for our vendor module template
    Logger::GetInstance().EnableTag("XENA");

    //Save configuration to base class variables
    //sizeof configuration must be a multiple of 4 bytes
    vendorConfigurationPointer = &configuration;
    configurationLength = sizeof(XenaPodModuleConfiguration);

    //Set defaults
    ResetToDefaultConfiguration();
}

void XenaPodModule::ResetToDefaultConfiguration()
{

    logt("XENA", "Configuring module"); 

    //Set default configuration values
    configuration.moduleId = vendorModuleId;
    configuration.moduleActive = true;
    configuration.moduleVersion = XENA_POD_MODULE_CONFIG_VERSION;

    configuration.sensorMeasurementIntervalDs = DEFAULT_SENSOR_MEASUREMENT_INTERVAL_DS;

    //This line allows us to have different configurations of this module depending on the featureset
    SET_FEATURESET_CONFIGURATION_VENDOR(&configuration, this);

    // make sure custom pinsets are available
    const auto getCustomPinset = Boardconf::GetInstance().getCustomPinset;
    if (!getCustomPinset)
    {
        logt(
            "XENA",
            "error: no custom pinsets available (potential "
            "mismatch between board id / board type and "
            "featureset)"
        );
        return;//ErrorType::NOT_SUPPORTED;
    }

    // Setup I2C pins
    I2CPins pins_i2c;
    pins_i2c.pinsetIdentifier = PinsetIdentifier::I2C;
    getCustomPinset(&pins_i2c);

    if (!FruityHal::TwiIsInitialized())
    {
        FruityHal::TwiInit(
            pins_i2c.sclPin,
            pins_i2c.sdaPin
        );
    }

    u8 data[4];

    FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__CHIP_ID, data, 1);
    logt("XENA", "CHIP_ID: %d", data[0]);

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

    // 1x Oversampling TP
    data[0] = BME688_REG__CTRL_MEAS;
    data[1] = 0x20;
    FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);

    // 1x Oversampling H
    data[0] = BME688_REG__CTRL_HUM;
    data[1] = 0x01;
    FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);

    // Run gas
    data[0] = BME688_REG__CTRL_GAS_1;
    data[1] = 0x10;
    FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);

    // Forced mode
    data[0] = BME688_REG__CTRL_MEAS;
    data[1] = 0x25;
    FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);

    /*

    // Setup SPI pins
    ST95HFPins pins_st95hf;
    pins_st95hf.pinsetIdentifier = PinsetIdentifier::ST95HF;
    getCustomPinset(&pins_st95hf);

    if (!FruityHal::SpiIsInitialized())
    {
        FruityHal::SpiInit(
            pins_st95hf.sckPin,
            pins_st95hf.misoPin,
            pins_st95hf.mosiPin
        );

    }

    nfc.init(pins_st95hf);

    {
        u8 buffer[3];
        nfc.command(ST95HF::Command::IDN, buffer, sizeof(buffer));
    }

    // Check data ready
    //logt("XENA", "INT %d", FruityHal::GpioPinRead(pins_st95hf.irqOutPin));

    {

        u8 rx[32];
        nfc.read(rx, 32);

        //char cbuffer[100];
        //Logger::ConvertBufferToHexString(rx, 32, cbuffer, sizeof(cbuffer));
        //logt("XENA", "Reply %s, len %u", cbuffer, 32);
        logt("XENA", "IDN: %s, ROMCRC: 0x%x", rx+3, (u16) ((rx[16] << 8 ) | rx[17]));

    }
    */

}

void XenaPodModule::ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength)
{
    // VendorModuleConfiguration* newConfig = (VendorModuleConfiguration*)migratableConfig;

    //Version migration can be added here, e.g. if module has version 2 and config is version 1
    // if(newConfig != nullptr && newConfig->moduleVersion == 1){/* ... */};

    // Find the index of the StatusReporterModule (if active) in order to use it for battery measurements.
    statusReporterModule = static_cast<StatusReporterModule *>(
        GS->node.GetModuleById(ModuleId::STATUS_REPORTER_MODULE)
    );

}

void XenaPodModule::TimerEventHandler(u16 passedTimeDs)
{

    [this] {
        const auto currentTime = GS->appTimerDs;

        if (currentTime - lastMeasurementAppTimer <= configuration.sensorMeasurementIntervalDs)
            return;

        logt("XENA", "Measuring"); 

        // read the sensor data
        u8 rxData[8];
        {

            const auto err2 = FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__MEAS_STATUS_0, rxData, 1);

            logt("XENA", "Measurment Status: %d", rxData[0]); 

            const auto err = FruityHal::TwiRegisterRead(BME688_ADDR, BME688_REG__PRESS_MSB_0, rxData, 8);

            // if the read failed due to the sensor being in the wrong state
            // or no valid measurement being present, skip restarting the
            // measurement
            if (err == ErrorType::FORBIDDEN || err == ErrorType::BUSY){
                return;
            }

            lastMeasurementAppTimer = currentTime;

            // if the measurement failed for any other reason skip sending
            // a message with potentially invalid data
            if (err != ErrorType::SUCCESS){
                return;
            }
        }

        //statusReporterModule ? statusReporterModule->GetBatteryVoltage() : 0xFF

        //FruityHal::PrintString("MEAS: ");
        //FruityHal::PrintNumber(*((u32*)rxData));
        //FruityHal::PrintNumber(*((u32*)(rxData+4)));

        u32 temp_adc = (rxData[3] << 12 ) | (rxData[4] << 4 ) | (rxData[5] >> 4);

        i32 t_fine;
        int16_t calc_temp;
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

            logt("XENA", "Temperature: %d", calc_temp);

        }

        u32 pres_adc = (rxData[0] << 12 ) | (rxData[1] << 4 ) | (rxData[2] >> 4);

        int32_t pressure_comp;
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

            logt("XENA", "Pressure: %u", (u32) pressure_comp);

        }

        u16 hum_adc = (rxData[6] << 8 ) | rxData[7];

        int32_t calc_hum;
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

            logt("XENA", "Humidity: %u", (u32) calc_hum);

        }

        // Start next measurment
        u8 data[4];
        data[0] = BME688_REG__CTRL_MEAS;
        data[1] = 0x25;
        FruityHal::TwiRegisterWrite(BME688_ADDR, data, 2);


        u8 txData[10];
        memcpy(txData + 0, &calc_temp, sizeof(calc_temp));
        memcpy(txData + 2, &pressure_comp, sizeof(pressure_comp));
        memcpy(txData + 6, &calc_hum, sizeof(calc_hum));

        SendModuleActionMessage(
            MessageType::MODULE_RAW_DATA_LIGHT, // message type
            NODE_ID_BROADCAST,                  // destination node id
            (u8)TriggerActionType::TAP_DETECTED,
            0,                                  // request handle
            (const u8*)txData,                  // data pointer
            sizeof(txData),                     // data length
            false                               // reliable
        );

        logt("XENA", "sensor data sent to node %u", (u32)NODE_ID_BROADCAST);

    }();

}

void XenaPodModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader)
{
    //Must call superclass for handling
    Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

}

#endif // IS_ACTIVE(XENA_POD_MODULE)
