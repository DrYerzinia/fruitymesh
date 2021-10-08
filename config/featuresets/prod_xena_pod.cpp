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
#include <Config.h>
#include <Logger.h>
#include <DebugModule.h>
#include <StatusReporterModule.h>
#include <BeaconingModule.h>
#include <ScanningModule.h>
#include <EnrollmentModule.h>
#include <IoModule.h>
#include <XenaPodModule.h>
#include <MeshAccessModule.h>
#include <GlobalState.h>

extern void SetBoard_32(BoardConfiguration* c);

void SetBoardConfiguration_prod_xena_pod(BoardConfiguration* c)
{
    // This sets the board type to 32 (XenaPod). In general the boardType is
    // programmed into a field of the UICR and it is not recommended to
    // hard-code it into the firmware like this.
    c->boardType = 32;

    SetBoard_32(c);
}

void SetFeaturesetConfiguration_prod_xena_pod(ModuleConfiguration* config, void* module)
{
    if (config->moduleId == ModuleId::CONFIG)
    {
        Conf::GetInstance().defaultLedMode = LedMode::OFF;
        Conf::GetInstance().terminalMode = TerminalMode::PROMPT;

        // Set the discovery timeout by default to reduce battery usage.
        Conf::GetInstance().highDiscoveryTimeoutSec = 60;

        // Set a long-term connection interval which gets used after the connection has been established for some time.
        Conf::GetInstance().meshMinLongTermConnectionInterval =
            MSEC_TO_UNITS(90, CONFIG_UNIT_1_25_MS);
        Conf::GetInstance().meshMaxLongTermConnectionInterval =
            MSEC_TO_UNITS(90, CONFIG_UNIT_1_25_MS);
    }
    else if (config->moduleId == ModuleId::NODE)
    {
        //Specifies a default enrollment for the github configuration
        //This is just for illustration purpose so that all nodes are enrolled and connect to each other after flashing
        //For production, all nodes should have a unique nodeKey in the UICR and should be unenrolled
        //They can then be enrolled by the user e.g. by using a smartphone application
        //More info is available as part of the documentation in the Specification and the UICR chapter
        NodeConfiguration* c = (NodeConfiguration*) config;
        //Default state will be that the node is already enrolled
        c->enrollmentState = EnrollmentState::ENROLLED;
        //Enroll the node by default in networkId 11
        c->networkId = 11;
        //Set a default network key of 22:22:22:22:22:22:22:22:22:22:22:22:22:22:22:22
        CheckedMemcpy(c->networkKey, "\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22\x22", 16);

        //Info: The default node key and other keys are set in Conf::LoadDefaults()
    }
}

void SetFeaturesetConfigurationVendor_prod_xena_pod(VendorModuleConfiguration* config, void* module)
{
    
}

u32 InitializeModules_prod_xena_pod(bool createModule)
{
    u32 size = 0;
    size += GS->InitializeModule<DebugModule>(createModule);
    size += GS->InitializeModule<StatusReporterModule>(createModule);
    size += GS->InitializeModule<BeaconingModule>(createModule);
    size += GS->InitializeModule<ScanningModule>(createModule);
    size += GS->InitializeModule<EnrollmentModule>(createModule);
    size += GS->InitializeModule<IoModule>(createModule);
    size += GS->InitializeModule<XenaPodModule>(createModule, RECORD_STORAGE_RECORD_ID_VENDOR_MODULE_CONFIG_BASE + 0);
    size += GS->InitializeModule<MeshAccessModule>(createModule);
    return size;
}

DeviceType GetDeviceType_prod_xena_pod()
{
    return DeviceType::STATIC;
}

Chipset GetChipset_prod_xena_pod()
{
    return Chipset::CHIP_NRF52833;
}

FeatureSetGroup GetFeatureSetGroup_prod_xena_pod()
{
    return FeatureSetGroup::XENA_POD;
}

u32 GetWatchdogTimeout_prod_xena_pod()
{
    return 32768UL * 10; //set to 0 if the watchdog should be disabled
}

u32 GetWatchdogTimeoutSafeBoot_prod_xena_pod()
{
    return 0;
}
