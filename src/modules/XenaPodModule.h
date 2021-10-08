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

#pragma once

#include "FruityHalBleGap.h"
#include "Utility.h"
#include <Module.h>
#include <GlobalState.h>
#include <type_traits>

#if IS_ACTIVE(XENA_POD_MODULE)

class StatusReporterModule;

constexpr u8 XENA_POD_ID_LSB = 0xFF;
constexpr u8 XENA_POD_ID_MSB = 0xFF;

// XENA POD
constexpr VendorModuleId XENA_POD_MODULE_ID = GET_VENDOR_MODULE_ID(
  (XENA_POD_ID_MSB << 8) |XENA_POD_ID_LSB, 1
);

constexpr u8 XENA_POD_MODULE_CONFIG_VERSION = 1;

#pragma pack(push)
#pragma pack(1)
struct XenaPodModuleConfiguration : VendorModuleConfiguration {
    /// Interval at which sensor data is read in deci-seconds.
    u16 sensorMeasurementIntervalDs;
    u16 padding;
};
static_assert(
    sizeof(XenaPodModuleConfiguration) % 4 == 0,
    "size of the module configuration must be a multiple of 4"
);
#pragma pack(pop)

class XenaPodModule : public Module
{

    enum class TriggerActionType : u8
    {
        SENSOR_DATA = 1,
        TAP_DETECTED = 2
    };

    enum class ActionResponseType : u8
    {
    };

private:
    std::array<u8, 64> transmissionBuffer;

    u32 lastMeasurementAppTimer = 0;

    StatusReporterModule *statusReporterModule = nullptr;

    u16 measurementSequenceNumber = 0;

public:
    DECLARE_CONFIG_AND_PACKED_STRUCT(XenaPodModuleConfiguration);

    XenaPodModule();

    void ConfigurationLoadedHandler(u8* migratableConfig, u16 migratableConfigLength) override final;

    void ResetToDefaultConfiguration() override final;

    void TimerEventHandler(u16 passedTimeDs) override final;

    void MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, ConnPacketHeader const * packetHeader) override;

};

#endif // IS_ACTIVE(XENA_POD_MODULE)
