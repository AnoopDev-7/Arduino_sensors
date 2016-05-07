/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef MyTransport_h
#define MyTransport_h

#define TIMEOUT_FAILURE_STATE 10000
// Search for a new parent node after this many transmission failures
#define TRANSMISSION_FAILURES 5	// max 15
// maximal mumber of hops for ping/pong
#define MAX_HOPS 254
#define INVALID_HOPS 255

#define AUTO 0xFF // 0-254. Id 255 is reserved for auto initialization of nodeId.

#define BROADCAST_ADDRESS ((uint8_t)0xFF)

// invalid distance when searching for parent
#define DISTANCE_INVALID (0xFF)

#define _autoFindParent (bool)(MY_PARENT_NODE_ID == AUTO)
#define isValidDistance(distance) (bool)(distance!=DISTANCE_INVALID)
#define isValidParent(parent) (bool)(parent != AUTO)

/// @brief Type of transport fsm
typedef enum {
	tsTRANSPORT_INIT, //!< tbd
	tsPARENT, //!< tbd
	tsID, //!< tbd
	tsLINK, //!< tbd
	tsREGISTER, //!< tbd
	tsOK, //!< tbd
	tsFAILURE
} transportStates;

/// @brief transport status
typedef struct {
	transportStates transportState:3; //!< fsm status
	bool nodeRegistered:1; //!< flag indicating node registered at GW, i.e. (for the moment) I_CONFIG received
	bool findingParentNode:1; //!< 	flag finding parent node is active
	bool preferredParentFound:1; //!< flag preferred parent found
	bool pingActive:1; //!< flag ping active
	bool pongReceived:1; //!< flag pong received
	uint8_t failedDownlinkTransmissions:4; //!< counter for failed downlink transmissions
	uint8_t failedUplinkTransmissions:4; //!< counter for failed uplink transmissions
	uint32_t heartbeat; //!< heartbeat counter, increments with every message sent
} __attribute__((packed)) transportStatus;


// Common functions in all radio drivers
void transportInitialize();
void transportStateMachine();
void transportProcess();

bool transportRequestNodeId();
bool transportPresentNode();
bool transportFindParentNode();

bool transportSendRoute(MyMessage &message);
bool transportSendWrite(uint8_t to, MyMessage &message);
uint32_t transportGetHeartbeat();
void transportWait(unsigned long ms);
void transportWait(unsigned long ms, uint8_t cmd, uint8_t msgtype);

// "Interface" functions for radio driver
bool transportInit();	// rename
void transportSetAddress(uint8_t address);
uint8_t transportGetAddress();
bool transportSend(uint8_t to, const void* data, uint8_t len);
bool transportAvailable(uint8_t *to);
uint8_t transportReceive(void* data);
void transportPowerDown();

#endif
