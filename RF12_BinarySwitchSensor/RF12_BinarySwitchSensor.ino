/**
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
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Simple binary switch example 
 * Connect button or door/window reed switch between 
 * digitial I/O pin 3 (BUTTON_PIN below) and GND.
 * http://www.mysensors.org/build/binary
 */


// use JeeLib as RFM12 API
#include <JeeLib.h>
// Translate between JeeLib and MyTransport  Modules
#include <MyTransportRFM12.h>

#include <MySensor.h>
#include <SPI.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define CHILD_ID 3
#define BUTTON_PIN  3  // Arduino Digital I/O pin for button/reed switch

// Set RFM12 Frequency (default: 433MHz) and NetworkID (default: 99)
MyTransportRFM12 transport(RFM12_FREQUENCY, RFM12_NETWORKID);
MySensor gw(transport);

Bounce debouncer = Bounce(); 
int oldValue=-1;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(CHILD_ID,V_TRIPPED);

void setup()  
{  
  gw.begin();

  Serial.println("Started clearing. Please wait...");
  for (int i=0;i<512;i++) {
    EEPROM.write(i, 0xff);
  }
  Serial.println("Clering done. You're ready to go!");
  
  
 // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  gw.present(CHILD_ID, S_DOOR);  
}


//  Check if digital input has changed and send in new value
void loop() 
{
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
 
  if (value != oldValue) {
     // Send in the new value
     gw.send(msg.set(value==HIGH ? 1 : 0));
     oldValue = value;
  }
} 

