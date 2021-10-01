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
    constexpr u16 DEFAULT_SENSOR_MEASUREMENT_INTERVAL_DS = 10;

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

// CTRL_REG2 0x04 HP
// CTRL_REG4 0x80 BDU

#define IIS2DH_ADDR              0x19

#define IIS2DH_REG__WHO_AM_I     0x0F
#define IIS2DH_REG__CTRL_REG_1   0x20
#define IIS2DH_REG__CTRL_REG_2   0x21
#define IIS2DH_REG__CTRL_REG_3   0x22
#define IIS2DH_REG__CTRL_REG_4   0x23
#define IIS2DH_REG__CTRL_REG_5   0x24
#define IIS2DH_REG__CTRL_REG_6   0x26
#define IIS2DH_REG__CLICK_CFG    0x38
#define IIS2DH_REG__CLICK_SRC    0x39
#define IIS2DH_REG__CLICK_THS    0x3A
#define IIS2DH_REG__TIME_LIMIT   0x3B
#define IIS2DH_REG__TIME_LATENCY 0x3C
#define IIS2DH_REG__TIME_WINDOW  0x3D

void XenaPodModule::ResetToDefaultConfiguration()
{
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

    // load the pinset for the BME280 sensor
    Lis2dh12Pins pins;
    pins.pinsetIdentifier = PinsetIdentifier::LIS2DH12;
    getCustomPinset(&pins);

    if (!FruityHal::TwiIsInitialized())
    {
        FruityHal::TwiInit(
            pins.sckPin,
            pins.sdaPin
        );
    }

    u8 data[2];

    // 5.376kHz Low-power Mode XYZ Enabled
    data[0] = IIS2DH_REG__CTRL_REG_1;
    data[1] = 0x5F;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    // TODO High Pass Filter CTRL_REG_2

    data[0] = IIS2DH_REG__CLICK_THS;
    data[1] = 0x60;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    data[0] = IIS2DH_REG__TIME_LIMIT;
    data[1] = 0x05;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    data[0] = IIS2DH_REG__TIME_LATENCY;
    data[1] = 0x80;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

    // Enable Z Single Click Interrupt
    data[0] = IIS2DH_REG__CLICK_CFG;
    data[1] = 0x10;
    FruityHal::TwiRegisterWrite(IIS2DH_ADDR, data, 2);

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

        // read the sensor data
        u8 rxData;
        {

            const auto err = FruityHal::TwiRegisterRead(IIS2DH_ADDR, IIS2DH_REG__CLICK_SRC, &rxData, 1);


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

        if( rxData & 0x04 ){
            SendModuleActionMessage(
                MessageType::COMPONENT_SENSE,       // message type
                NODE_ID_BROADCAST,                  // destination node id
                (u8)TriggerActionType::TAP_DETECTED,    
                0,                                  // request handle
                (const u8*)&rxData,                 // data pointer
                sizeof(1),                          // data length
                false                               // reliable
            );
            FruityHal::PrintString("TAP");
        }

        logt("XENA", "sensor data sent to node %u", (u32)NODE_ID_BROADCAST);
    }();

}

void XenaPodModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader)
{
    //Must call superclass for handling
    Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

}

#endif // IS_ACTIVE(XENA_POD_MODULE)
