/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#include "MyTransportHAL.h"
#include "drivers/RFM95/RFM95.h"

#if defined(MY_RFM95_ENABLE_ENCRYPTION)

#include "drivers/Crypto/Crypto.cpp"
#include "drivers/Crypto/OMAC.cpp"
#include "drivers/Crypto/AuthenticatedCipher.cpp"
#include "drivers/Crypto/Cipher.cpp"
#include "drivers/Crypto/GF128.cpp"
#include "drivers/Crypto/EAX.cpp"
#include "drivers/Crypto/AES256.cpp"
#include "drivers/Crypto/AESCommon.cpp"
#include "drivers/Crypto/BlockCipher.cpp"

#define IV_SIZE 16
#define TAG_SIZE 16
#define PSK_SIZE 16

#endif

#if defined(MY_RFM95_ENABLE_ENCRYPTION)
EAX<AES256> eax;
uint8_t eaxBuffer[64];
#endif

bool transportInit(void)
{
#if defined(MY_RFM95_ENABLE_ENCRYPTION)
	hwRandomNumberInit();
	uint8_t _psk[PSK_SIZE];
	//hwReadConfigBlock((void*)_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
	(void)memset(_psk, 0xAA, sizeof(_psk));
	eax.setKey(&_psk[0], sizeof(_psk));
	(void)memset(_psk, 0, sizeof(_psk));
#endif
	const bool result = RFM95_initialise(MY_RFM95_FREQUENCY);
#if defined(MY_RFM95_TCXO)
	RFM95_enableTCXO();
#endif
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM95_ATC_MODE_DISABLED)
	// only enable ATC mode in nodes
	RFM95_ATCmode(true, MY_RFM95_ATC_TARGET_RSSI);
#endif
	return result;
}

void transportSetAddress(const uint8_t address)
{
	RFM95_setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return RFM95_getAddress();
}

bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool noACK)
{
#if defined(MY_RFM95_ENABLE_ENCRYPTION)
	const uint8_t finalLength = len > 16 ? 32 : 16;
	// randomize padding bytes - not really needed
	for (uint8_t i = 0; i < IV_SIZE + finalLength; i++) {
		eaxBuffer[i] = random(255);
	}
	(void)memcpy(&eaxBuffer[IV_SIZE], data, len);

	// add addressing information to MAC
	eax.setIV(eaxBuffer, IV_SIZE);
	uint8_t addressing[] = { to };

	eax.addAuthData(addressing, sizeof(addressing));
	eax.encrypt(eaxBuffer + IV_SIZE, &eaxBuffer[IV_SIZE], finalLength);
	eax.computeTag(eaxBuffer + IV_SIZE + finalLength, TAG_SIZE);

	/*
	MY_SERIALDEVICE.println("IV");
	for (uint8_t i = 0; i < IV_SIZE; i++) {
		MY_SERIALDEVICE.print(eaxBuffer[i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();
	MY_SERIALDEVICE.println("PT");
	for (uint8_t i = 0; i < len; i++) {
		MY_SERIALDEVICE.print(*((uint8_t*)data+i), HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();
	MY_SERIALDEVICE.println("CT");
	for (uint8_t i = 0; i < len; i++) {
		MY_SERIALDEVICE.print(eaxBuffer[IV_SIZE +i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();
	MY_SERIALDEVICE.println("TAG");
	for (uint8_t i = 0; i < TAG_SIZE; i++) {
		MY_SERIALDEVICE.print(eaxBuffer[IV_SIZE + finalLength + i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();
	*/

	if (noACK) {
		(void)RFM95_sendWithRetry(to, eaxBuffer, finalLength + IV_SIZE + TAG_SIZE, 0, 0);
		return true;
	}
	return RFM95_sendWithRetry(to, eaxBuffer, finalLength + IV_SIZE + TAG_SIZE);
#else
	if (noACK) {
		(void)RFM95_sendWithRetry(to, data, len, 0, 0);
		return true;
	}
	return RFM95_sendWithRetry(to, data, len);
#endif
}

bool transportAvailable(void)
{
	return RFM95_available();
}

bool transportSanityCheck(void)
{
	return RFM95_sanityCheck();
}

uint8_t transportReceive(void* data)
{
	uint8_t len = RFM95_recv(eaxBuffer, sizeof(eaxBuffer));

#if defined(MY_RFM95_ENABLE_ENCRYPTION)
	eax.setIV(eaxBuffer, IV_SIZE);
	uint8_t addressing[] = { RFM95_getAddress() };
	eax.addAuthData(addressing, sizeof(addressing));
	eax.decrypt((uint8_t*)data, &eaxBuffer[IV_SIZE], (len - IV_SIZE - TAG_SIZE));
	// verify authenticity + integrity
	if (!eax.checkTag(&eaxBuffer[len - TAG_SIZE], TAG_SIZE)) {
		len = 0;
		(void)memset(eaxBuffer, 0xFF, sizeof(eaxBuffer));
		MY_SERIALDEVICE.println("bad data");
	}

	/*
	MY_SERIALDEVICE.print("LEN: ");
	MY_SERIALDEVICE.println(len);

	MY_SERIALDEVICE.println("cipher text");
	for (uint8_t i = 0; i < (len - IV_SIZE - TAG_SIZE); i++) {
		MY_SERIALDEVICE.print(eaxBuffer[IV_SIZE + i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();


	MY_SERIALDEVICE.println("IV");
	for (uint8_t i = 0; i < IV_SIZE; i++) {
		MY_SERIALDEVICE.print(eaxBuffer[i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();


	MY_SERIALDEVICE.println("tag");
	for (uint8_t i = 0; i < TAG_SIZE; i++) {
		MY_SERIALDEVICE.print(eaxBuffer[len - IV_SIZE - TAG_SIZE + i], HEX);
		MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();
	MY_SERIALDEVICE.println("clear text");
	for (uint8_t i = 0; i <  (len - IV_SIZE - TAG_SIZE); i++) {
	MY_SERIALDEVICE.print(*((uint8_t*)data + i), HEX);
	MY_SERIALDEVICE.print(" ");
	}
	MY_SERIALDEVICE.println();

	*/




#endif
	return len;
}

void transportSleep(void)
{
	(void)RFM95_sleep();
}

void transportStandBy(void)
{
	(void)RFM95_standBy();
}

void transportPowerDown(void)
{
	RFM95_powerDown();
}

void transportPowerUp(void)
{
	RFM95_powerUp();
}

void transportToggleATCmode(const bool OnOff, const int16_t targetRSSI)
{
	RFM95_ATCmode(OnOff, targetRSSI);
}

int16_t transportGetSendingRSSI(void)
{
	return RFM95_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	return RFM95_getReceivingRSSI();
}

int16_t transportGetSendingSNR(void)
{
	return static_cast<int16_t>(RFM95_getSendingSNR());
}

int16_t transportGetReceivingSNR(void)
{
	return static_cast<int16_t>(RFM95_getReceivingSNR());
}

int16_t transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(RFM95_getTxPowerPercent());
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(RFM95_getTxPowerLevel());
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RFM95_setTxPowerPercent(powerPercent);
}

