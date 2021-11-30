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
    constexpr u16 DEFAULT_SENSOR_MEASUREMENT_INTERVAL_DS = 100;

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

        if (currentTime - lastBroadcastMeasurementAppTimer >= configuration.sensorMeasurementIntervalDs)
        {

            lastBroadcastMeasurementAppTimer = currentTime;

            u8 txData[] = "Hello World!";

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

        }

    }();

}

void XenaPodModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader)
{
    //Must call superclass for handling
    Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

}

#endif // IS_ACTIVE(XENA_POD_MODULE)
