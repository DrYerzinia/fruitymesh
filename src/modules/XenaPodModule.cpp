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

    u8 chip_id = bme688.init();
    logt("XENA", "BME688 CHIPID: %d", chip_id);

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

        i16 temp;
        i32 pressure;
        i32 humidity;

        const auto err = bme688.read_sensors(temp, pressure, humidity);

        if(err == ErrorType::SUCCESS) {
            lastMeasurementAppTimer = currentTime;
        } else {
            return;
        }

        logt("XENA", "Temperature: %d.%d C", temp / 100, temp % 100);
        logt("XENA", "Pressure: %u.%u hPa", (u32) pressure / 100, pressure % 100);
        logt("XENA", "Humidity: %u.%u %%RH", (u32) humidity / 1000, humidity % 1000);

        //statusReporterModule ? statusReporterModule->GetBatteryVoltage() : 0xFF

        bme688.start_measurment();

        u8 txData[10];
        memcpy(txData + 0, &temp, sizeof(temp));
        memcpy(txData + 2, &pressure, sizeof(pressure));
        memcpy(txData + 6, &humidity, sizeof(humidity));

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
