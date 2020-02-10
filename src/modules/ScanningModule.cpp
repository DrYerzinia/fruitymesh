////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2019 M-Way Solutions GmbH
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


#include <ScanningModule.h>


#include <Logger.h>
#include <ScanController.h>
#include <Utility.h>
#include <Node.h>
#include <stdlib.h>
#include <GlobalState.h>
constexpr u8 SCAN_MODULE_CONFIG_VERSION = 2;

#if IS_ACTIVE(ASSET_MODULE)
#ifndef GITHUB_RELEASE
#include <AssetModule.h>
#endif //GITHUB_RELEASE
#endif

#define SCANNING_MODULE_AVERAGE_RSSI(COUNT, RSSI, NEW_RSSI) (((u32)((RSSI) * (COUNT)) + (NEW_RSSI))/((COUNT) + 1))


//This module scans for specific messages and reports them back
//This implementation is currently very basic and should just illustrate how
//such functionality could be implemented

ScanningModule::ScanningModule() :
		Module(ModuleId::SCANNING_MODULE, "scan")
{
	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(ScanningModuleConfiguration);

	//Initialize scanFilters as empty
	for (int i = 0; i < SCAN_FILTER_NUMBER; i++)
	{
		scanFilters[i].active = 0;
	}

	//resetAssetTrackingTable();

	assetInsPackets.zeroData();

	//Set defaults
	ResetToDefaultConfiguration();
}

void ScanningModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = SCAN_MODULE_CONFIG_VERSION;

	//TODO: This is for testing only
	scanFilterEntry filter;

	filter.grouping = GroupingType::GROUP_BY_ADDRESS;
	filter.address.addr_type = FruityHal::BleGapAddrType::INVALID;
	filter.advertisingType = 0xFF;
	filter.minRSSI = -100;
	filter.maxRSSI = 100;

	SET_FEATURESET_CONFIGURATION(&configuration, this);
}

void ScanningModule::ConfigurationLoadedHandler(ModuleConfiguration* migratableConfig, u16 migratableConfigLength)
{
	//Do additional initialization upon loading the config

	// Reset address pointer to the beginning of the address table
//	resetAddressTable();
//	resetTotalRSSIsPerAddress();
//	resetTotalMessagesPerAdress();

	totalMessages = 0;
	totalRSSI = 0;

	assetPackets.zeroData();

#if IS_INACTIVE(GW_SAVE_SPACE)
	if (configuration.moduleActive && assetReportingIntervalDs != 0) {
		GS->scanController.UpdateJobPointer(&p_scanJob, ScanState::HIGH, ScanJobState::ACTIVE);
	}
#endif
	//Start the Module...
}

#ifdef TERMINAL_ENABLED
TerminalCommandHandlerReturnType ScanningModule::TerminalCommandHandler(const char* commandArgs[], u8 commandArgsSize)
{
	//Must be called to allow the module to get and set the config
	return Module::TerminalCommandHandler(commandArgs, commandArgsSize);
}
#endif

void ScanningModule::TimerEventHandler(u16 passedTimeDs)
{
	if(SHOULD_IV_TRIGGER(GS->appTimerDs, passedTimeDs, groupedReportingIntervalDs))
	{

		//Send grouped packets
//		SendReport();
//
//		resetAddressTable();
//		resetTotalRSSIsPerAddress();
//		resetTotalMessagesPerAdress();

		totalMessages = 0;
		totalRSSI = 0;
	}

	if(SHOULD_IV_TRIGGER(GS->appTimerDs, passedTimeDs, assetReportingIntervalDs)){
		//Send asset tracking packets
		SendTrackedAssets();
		SendTrackedAssetsIns();
//		resetAssetTrackingTable();
	}
}

void ScanningModule::MeshMessageReceivedHandler(BaseConnection* connection, BaseConnectionSendData* sendData, connPacketHeader* packetHeader)
{
	//Must call superclass for handling
	Module::MeshMessageReceivedHandler(connection, sendData, packetHeader);

	if(packetHeader->messageType == MessageType::ASSET_V2)
	{
		ScanModuleTrackedAssetsV2Message* packet = (ScanModuleTrackedAssetsV2Message*) packetHeader;

		ReceiveTrackedAssets(sendData, packet);
	}
	else if (packetHeader->messageType == MessageType::ASSET_GENERIC && sendData->dataLength >= SIZEOF_CONN_PACKET_MODULE)
	{
		connPacketModule *connPacket = (connPacketModule*)packetHeader;
		if (connPacket->actionType == (u8)ScanModuleMessages::ASSET_INS_TRACKING_PACKET)
		{
			TrackedAssetInsMessage* msg = (TrackedAssetInsMessage*)connPacket->data;
			u32 amount = (sendData->dataLength - SIZEOF_CONN_PACKET_MODULE) / sizeof(TrackedAssetInsMessage);
			ReceiveTrackedAssetsIns(msg, amount, packetHeader->sender);
		}
	}
}


void ScanningModule::GapAdvertisementReportEventHandler(const FruityHal::GapAdvertisementReportEvent& advertisementReportEvent)
{
	if (!configuration.moduleActive) return;

#if IS_INACTIVE(GW_SAVE_SPACE)
	HandleAssetV2Packets(advertisementReportEvent);
	HandleAssetInsPackets(advertisementReportEvent);
#endif
}

#define _______________________ASSET_V2______________________

#if IS_INACTIVE(GW_SAVE_SPACE)
//This function checks whether we received an assetV2 packet
void ScanningModule::HandleAssetV2Packets(const FruityHal::GapAdvertisementReportEvent& advertisementReportEvent)
{
	const advPacketServiceAndDataHeader* packet = (const advPacketServiceAndDataHeader*)advertisementReportEvent.getData();
	const advPacketAssetServiceData* assetPacket = (const advPacketAssetServiceData*)&packet->data;

	//Check if the advertising packet is an asset packet
	if (
			advertisementReportEvent.getDataLength() >= SIZEOF_ADV_STRUCTURE_ASSET_SERVICE_DATA
			&& packet->flags.len == SIZEOF_ADV_STRUCTURE_FLAGS-1
			&& packet->uuid.len == SIZEOF_ADV_STRUCTURE_UUID16-1
			&& packet->data.uuid.type == BLE_GAP_AD_TYPE_SERVICE_DATA
			&& packet->data.uuid.uuid == SERVICE_DATA_SERVICE_UUID16
			&& packet->data.messageType == ServiceDataMessageType::STANDARD_ASSET
	){
		char serial[6];
		Utility::GenerateBeaconSerialForIndex(assetPacket->serialNumberIndex, serial);
		logt("SCANMOD", "RX ASSETV2 ADV: serial %s, pressure %u, speed %u, temp %u, humid %u, cn %u, rssi %d, nodeId %u",
			serial,
			assetPacket->pressure,
			assetPacket->speed,
			assetPacket->temperature,
			assetPacket->humidity,
			assetPacket->advertisingChannel,
			advertisementReportEvent.getRssi(),
			(u32)assetPacket->nodeId
		);

		if (assetPacket->serialNumberIndex != 0)
		{
			i8 rssi = advertisementReportEvent.getRssi();
			rssi = -rssi; //Make rssi positive
			if (rssi >= 10 && rssi <= 90) //filter out wrong rssis
			{
				//Adds the asset packet to our buffer
				addTrackedAsset(assetPacket, rssi);
			}
		}
	}
}

void ScanningModule::HandleAssetInsPackets(const FruityHal::GapAdvertisementReportEvent & advertisementReportEvent)
{
	const advPacketServiceAndDataHeader* packet = (const advPacketServiceAndDataHeader*)advertisementReportEvent.getData();
	const advPacketAssetInsServiceData* assetPacket = (const advPacketAssetInsServiceData*)&packet->data;

	//Check if the advertising packet is an asset packet
	if (
		advertisementReportEvent.getDataLength() >= SIZEOF_ADV_STRUCTURE_ASSET_INS_SERVICE_DATA
		&& packet->flags.len == SIZEOF_ADV_STRUCTURE_FLAGS - 1
		&& packet->uuid.len == SIZEOF_ADV_STRUCTURE_UUID16 - 1
		&& packet->data.uuid.type == BLE_GAP_AD_TYPE_SERVICE_DATA
		&& packet->data.uuid.uuid == SERVICE_DATA_SERVICE_UUID16
		&& packet->data.messageType == ServiceDataMessageType::INS_ASSET
		) {
		logt("SCANMOD", "RX ASSETV2 ADV: nodeId %u, batteryPower %u, absolutePositionX %u, absolutePositionY %u, pressure %u, rssi %d", 
			assetPacket->assetNodeId,
			assetPacket->batteryPower,
			assetPacket->absolutePositionX,
			assetPacket->absolutePositionY,
			assetPacket->pressure,
			advertisementReportEvent.getRssi());

		i8 rssi = advertisementReportEvent.getRssi();
		rssi = -rssi; //Make rssi positive
		if (rssi >= 10 && rssi <= 90) //filter out wrong rssis
		{
			//Adds the asset packet to our buffer
			addTrackedAssetIns(assetPacket, rssi);
		}
	}
}

/**
 * Finds a free slot in our buffer of asset packets and adds the packet
 */
bool ScanningModule::addTrackedAsset(const advPacketAssetServiceData* packet, i8 rssi){
	ScannedAssetTrackingStorage* slot = nullptr;

	//Look for an old entry of this asset or a free space
	//Because we fill this buffer from the beginning, we can use the first slot that is empty
	for(int i = 0; i<ASSET_PACKET_BUFFER_SIZE; i++){
		if(assetPackets[i].serialNumberIndex == packet->serialNumberIndex || assetPackets[i].serialNumberIndex == 0){
			slot = &assetPackets[i];
			break;
		}
	}

	//If a slot was found, add the packet
	if(slot != nullptr){
		u16 slotNum = ((u32)slot - (u32)assetPackets.getRaw()) / sizeof(ScannedAssetTrackingStorage);
		logt("SCANMOD", "Tracked packet %u in slot %d", packet->serialNumberIndex, slotNum);

		//Clean up first, if we overwrite another assetId
		if(slot->serialNumberIndex != packet->serialNumberIndex){
			slot->serialNumberIndex = packet->serialNumberIndex;
			slot->nodeId = packet->nodeId;
			slot->rssiContainer.count = 0;
			slot->rssiContainer.rssi37 = slot->rssiContainer.rssi38 = slot->rssiContainer.rssi39 = UINT8_MAX;
			slot->rssiContainer.channelCount[0] = slot->rssiContainer.channelCount[1] = slot->rssiContainer.channelCount[2] = 0;
		}
		slot->direction = packet->direction;
		slot->pressure = packet->pressure;
		slot->speed = packet->speed;

		RssiRunningAverageCalculationInPlace(slot->rssiContainer, packet->advertisingChannel, rssi);

		return true;
	}
	return false;
}
bool ScanningModule::addTrackedAssetIns(const advPacketAssetInsServiceData * packet, i8 rssi)
{
	ScannedAssetInsTrackingStorage* slot = nullptr;

	//Look for an old entry of this asset or a free space
	//Because we fill this buffer from the beginning, we can use the first slot that is empty
	for (int i = 0; i < ASSET_INS_PACKET_BUFFER_SIZE; i++) {
		if (assetInsPackets[i].assetNodeId == packet->assetNodeId || assetInsPackets[i].assetNodeId == 0) {
			slot = &assetInsPackets[i];
			break;
		}
	}

	//If a slot was found, add the packet
	if (slot != nullptr) {
		u16 slotNum = ((u32)slot - (u32)assetInsPackets.getRaw()) / sizeof(ScannedAssetInsTrackingStorage);
		logt("SCANMOD", "Tracked packet %u in slot %d", packet->assetNodeId, slotNum);

		//Clean up first, if we overwrite another assetId
		if (slot->assetNodeId != packet->assetNodeId) {
			slot->assetNodeId = packet->assetNodeId;
			slot->rssiContainer.count = 0;
			slot->rssiContainer.rssi37 = slot->rssiContainer.rssi38 = slot->rssiContainer.rssi39 = UINT8_MAX;
			slot->rssiContainer.channelCount[0] = slot->rssiContainer.channelCount[1] = slot->rssiContainer.channelCount[2] = 0;
		}
		slot->batteryPower = packet->batteryPower;
		slot->absolutePositionX = packet->absolutePositionX;
		slot->absolutePositionY = packet->absolutePositionY;
		slot->pressure = packet->pressure;
		slot->moving = packet->moving;

		RssiRunningAverageCalculationInPlace(slot->rssiContainer, 0, rssi);

		return true;
	}
	return false;
}
#endif

/**
 * Sends out all tracked assets from our buffer and resets the buffer
 */

//FIXME: rssi threshold must be used somewhere, apply when receiving packet?
//FIXME: do we average packets or do we just take the best rssi

void ScanningModule::SendTrackedAssets()
{
#if IS_INACTIVE(GW_SAVE_SPACE)
	//Find out how many assets were tracked
	u8 count = 0;
	for(int i=0; i<ASSET_PACKET_BUFFER_SIZE; i++){
		if(assetPackets[i].serialNumberIndex == 0) break;
		count++;
	}

	if(count == 0) return;

	//jstodo add test for tracked assets
	u16 messageLength = SIZEOF_CONN_PACKET_HEADER + SIZEOF_SCAN_MODULE_TRACKED_ASSET_V2 * count;

	//Allocate a buffer big enough and fill the packet
	DYNAMIC_ARRAY(buffer, messageLength);
	ScanModuleTrackedAssetsV2Message* message = (ScanModuleTrackedAssetsV2Message*) buffer;

	message->header.messageType = MessageType::ASSET_V2;
	message->header.sender = GS->node.configuration.nodeId;
	message->header.receiver = NODE_ID_SHORTEST_SINK;

	for(int i=0; i<count; i++){
		if (assetPackets[i].nodeId != 0)
		{
			message->trackedAssets[i].assetId = assetPackets[i].nodeId;
		}
		else
		{
			message->trackedAssets[i].assetId = assetPackets[i].serialNumberIndex;
		}
		message->trackedAssets[i].rssi37 = assetPackets[i].rssiContainer.rssi37;
		message->trackedAssets[i].rssi38 = assetPackets[i].rssiContainer.rssi38;
		message->trackedAssets[i].rssi39 = assetPackets[i].rssiContainer.rssi39;

		message->trackedAssets[i].speed = ConvertServiceDataToMeshMessageSpeed(assetPackets[i].speed);

		message->trackedAssets[i].direction = assetPackets[i].direction / 16; //TODO: convert meaningful
		message->trackedAssets[i].pressure = ConvertServiceDataToMeshMessagePressure(assetPackets[i].pressure);
	}

	//Send the packet as a non-module message to save some bytes in the header
	GS->cm.SendMeshMessage(
			buffer,
			(u8)messageLength,
			DeliveryPriority::LOW
			);

	//Clear the buffer
	assetPackets.zeroData();
#endif
}

void ScanningModule::SendTrackedAssetsIns()
{
#if IS_INACTIVE(GW_SAVE_SPACE)
	//Find out how many assets were tracked
	u8 count = 0;
	for (int i = 0; i < ASSET_INS_PACKET_BUFFER_SIZE; i++) {
		if (assetInsPackets[i].assetNodeId == 0) break;
		count++;
	}

	if (count == 0) return;

	//jstodo add test for tracked assets
	u16 messageLength = sizeof(TrackedAssetInsMessage) * count;

	//Allocate a buffer big enough and fill the packet
	DYNAMIC_ARRAY(buffer, messageLength);
	TrackedAssetInsMessage* trackedAssets = (TrackedAssetInsMessage*)buffer;

	for (int i = 0; i < count; i++) {
		trackedAssets[i].assetNodeId = assetInsPackets[i].assetNodeId;
		trackedAssets[i].rssi37 = assetInsPackets[i].rssiContainer.rssi37;
		trackedAssets[i].rssi38 = assetInsPackets[i].rssiContainer.rssi38;
		trackedAssets[i].rssi39 = assetInsPackets[i].rssiContainer.rssi39;
		trackedAssets[i].batteryPower = assetInsPackets[i].batteryPower;
		trackedAssets[i].absolutePositionX = assetInsPackets[i].absolutePositionX;
		trackedAssets[i].absolutePositionY = assetInsPackets[i].absolutePositionY;

		trackedAssets[i].moving = assetInsPackets[i].moving;
		trackedAssets[i].pressure = ConvertServiceDataToMeshMessagePressure(assetInsPackets[i].pressure);
	}

	SendModuleActionMessage(
		MessageType::ASSET_GENERIC,
		NODE_ID_SHORTEST_SINK,
		(u8)ScanModuleMessages::ASSET_INS_TRACKING_PACKET,
		0,
		buffer,
		messageLength,
		false
	);

	//Clear the buffer
	assetInsPackets.zeroData();
#endif
}

void ScanningModule::ReceiveTrackedAssets(BaseConnectionSendData* sendData, ScanModuleTrackedAssetsV2Message* packet) const
{
	u8 count = (sendData->dataLength - SIZEOF_CONN_PACKET_HEADER)  / SIZEOF_SCAN_MODULE_TRACKED_ASSET_V2;

	logjson("SCANMOD", "{\"nodeId\":%d,\"type\":\"tracked_assets\",\"assets\":[", packet->header.sender);

	for(int i=0; i<count; i++){
		trackedAssetV2* assetData = packet->trackedAssets + i;

		i8 speed = assetData->speed == 0xF ? -1 : assetData->speed;
		i8 direction = assetData->direction == 0xF ? -1 : assetData->direction;
		i16 pressure = assetData->pressure == 0xFF ? -1 : assetData->pressure; //(taken %250 to exclude 0xFF)

		if(i != 0) logjson("SCANMOD", ",");
		logjson("SCANMOD", "{\"id\":%u,\"rssi1\":%d,\"rssi2\":%d,\"rssi3\":%d,\"speed\":%d,\"direction\":%d,\"pressure\":%d}",
				assetData->assetId,
				assetData->rssi37,
				assetData->rssi38,
				assetData->rssi39,
				speed,
				direction,
				pressure);

		//logt("SCANMOD", "MESH RX id: %u, rssi %u, speed %u", assetData->assetId, assetData->rssi, assetData->speed);
	}

	logjson("SCANMOD", "]}" SEP);
}

void ScanningModule::ReceiveTrackedAssetsIns(TrackedAssetInsMessage *msg, u32 amount, NodeId sender) const
{
	logjson("SCANMOD", "{\"nodeId\":%d,\"type\":\"tracked_assets_ins\",\"assets\":[", sender);

	for (u32 i = 0; i < amount; i++) {

		i16 pressure = msg[i].pressure == 0xFF ? -1 : msg[i].pressure; //(taken %250 to exclude 0xFF)

		if (i != 0) logjson("SCANMOD", ",");
		logjson("SCANMOD", "{\"id\":%u,\"rssi1\":%d,\"rssi2\":%d,\"rssi3\":%d,\"batteryPower\":%u,\"absolutePositionX\":%u,\"absolutePositionY\":%u,\"moving\":%u,\"pressure\":%d}",
			msg[i].assetNodeId,
			msg[i].rssi37,
			msg[i].rssi38,
			msg[i].rssi39,
			msg[i].batteryPower,
			msg[i].absolutePositionX,
			msg[i].absolutePositionY,
			(u32)msg[i].moving,
			pressure);

	}

	logjson("SCANMOD", "]}" SEP);
}

void ScanningModule::RssiRunningAverageCalculationInPlace(RssiContainer &container, u8 advertisingChannel, i8 rssi)
{
	//If the count is at its max, we reset the rssi
	if (container.count == UINT8_MAX) {
		container.count = 0;
		container.rssi37 = container.rssi38 = container.rssi39 = UINT8_MAX;
		container.channelCount[0] = container.channelCount[1] = container.channelCount[2] = 0;
	}

	container.count++;
	//Channel 0 means that we have no channel data, add it to all rssi channels
	if (advertisingChannel == 0 && rssi < container.rssi37) {
		container.rssi37 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[0], container.rssi37, rssi);
		container.rssi38 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[1], container.rssi38, rssi);
		container.rssi39 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[2], container.rssi39, rssi);
		container.channelCount[0]++;
		container.channelCount[1]++;
		container.channelCount[2]++;
	}
	if (advertisingChannel == 1 && rssi < container.rssi37) {
		container.rssi37 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[0], container.rssi37, rssi);
		container.channelCount[0]++;
	}
	if (advertisingChannel == 2 && rssi < container.rssi38) {
		container.rssi38 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[1], container.rssi38, rssi);
		container.channelCount[1]++;
	}
	if (advertisingChannel == 3 && rssi < container.rssi39) {
		container.rssi39 = (u16)SCANNING_MODULE_AVERAGE_RSSI(container.channelCount[2], container.rssi39, rssi);
		container.channelCount[2]++;
	}
}

u8 ScanningModule::ConvertServiceDataToMeshMessageSpeed(u8 serviceDataSpeed)
{
	if      (serviceDataSpeed == 0xFF) return 0xF;
	else if (serviceDataSpeed > 140)   return 14;
	else if (serviceDataSpeed == 1)    return 1;
	else                               return serviceDataSpeed / 10;
}

u8 ScanningModule::ConvertServiceDataToMeshMessagePressure(u16 serviceDataPressure)
{
	if (serviceDataPressure == 0xFFFF) return 0xFF;
	else return (u8)(serviceDataPressure % 250); //Will wrap, which is ok (we still have a relative pressure, but not the absolute one, mod 250 to reserve 0xFF for not available)

}
