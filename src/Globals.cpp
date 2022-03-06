/*
  Helpers.cpp - Helper classes
  
  Copyright (C) 2020 - 2022  Davide Perini
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.
  
  You should have received a copy of the MIT License along with this program.  
  If not, see <https://opensource.org/licenses/MIT/>.
*/

#include "Globals.h"
#include "BootstrapManager.h"
#include "EffectsManager.h"
#include "LedManager.h"
#include "NetworkManager.h"

BootstrapManager bootstrapManager;

EffectsManager effectsManager;
LedManager ledManager;
NetworkManager networkManager;

Helpers helper;
Globals globals;

uint8_t prefix[] = {'D', 'P', 's', 'o', 'f', 't'}, hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp, fireflyEffect, fireflyColorMode, i;
uint8_t prefixLength = 6;
uint8_t gpioInUse = 2;
uint8_t colorMode = 0;
byte brightness = 255;
uint8_t whiteTempCorrection[] = {255, 255, 255};
Effect effect;
float framerate = 0;
float framerateCounter = 0;
uint lastStream = 0;
uint8_t baudRateInUse = 3;
bool relayState = false;
bool breakLoop = false;



/**
 * Set gpio received by the Firefly Luciferin software
 * @param gpio gpio to use
 */
void Globals::setGpio(int gpio) {

  Serial.println("CHANGING GPIO");
  gpioInUse = gpio;
#if defined(ESP8266)
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[GPIO_PARAM] = gpioInUse;
  bootstrapManager.writeToLittleFS(gpioDoc, GPIO_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[GPIO_PARAM] = gpioInUse;
  bootstrapManager.writeToSPIFFS(gpioDoc, GPIO_FILENAME);
#endif
  delay(20);

}

int Globals::setBaudRateInUse(int baudRate) {

  baudRateInUse = baudRate;
  int baudRateToUse = 0;
  switch (baudRate) {
    case 1: baudRateToUse = 230400; break;
    case 2: baudRateToUse = 460800; break;
    case 4: baudRateToUse = 921600; break;
    case 5: baudRateToUse = 1000000; break;
    case 6: baudRateToUse = 1500000; break;
    case 7: baudRateToUse = 2000000; break;
    default: baudRateToUse = 500000; break;
  }
  return baudRateToUse;

}

/**
 * Set baudRate received by the Firefly Luciferin software
 * @param baudRate int
 */
void Globals::setBaudRate(int baudRate) {

  Serial.println(F("CHANGING BAUDRATE"));
  setBaudRateInUse(baudRate);
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
  bootstrapManager.writeToLittleFS(baudrateDoc, BAUDRATE_FILENAME);
#endif
#if defined(ESP32)
  bootstrapManager.writeToSPIFFS(baudrateDoc, BAUDRATE_FILENAME);
  SPIFFS.end();
#endif
  delay(20);

}

/**
 * Turn ON the relay
 */
void Globals::turnOnRelay() {

  if (!relayState) {
    relayState = true;
#if defined(ESP8266)
    digitalWrite(RELAY_PIN, HIGH);
#elif defined(ESP32)
    digitalWrite(RELAY_PIN_DIG, HIGH);
    digitalWrite(RELAY_PIN_PICO, HIGH);
#endif
    delay(100);
  }

}

/**
 * Turn OFF the relay
 */
void Globals::turnOffRelay() {

  if (relayState) {
    relayState = false;
    delay(100);
#if defined(ESP8266)
    digitalWrite(RELAY_PIN, LOW);
#elif defined(ESP32)
    digitalWrite(RELAY_PIN_DIG, LOW);
    digitalWrite(RELAY_PIN_PICO, LOW);
#endif
  }

}

/**
 * Send serial info
 */
void Globals::sendSerialInfo() {

  EVERY_N_SECONDS(10) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    framerate = framerateCounter > 0 ? framerateCounter / 10 : 0;
    framerateCounter = 0;
    Serial.printf("framerate:%s\n", (String((framerate > 0.5 ? framerate : 0),1)).c_str());
    Serial.printf("firmware:%s\n", "LIGHT");
#else
    Serial.printf("firmware:%s\n", "FULL");
    Serial.printf("mqttopic:%s\n", networkManager.topicInUse.c_str());
#endif
    Serial.printf("ver:%s\n", VERSION);
    Serial.printf("lednum:%d\n", ledManager.dynamicLedNum);
#if defined(ESP32)
    Serial.printf("board:%s\n", "ESP32");
#elif defined(ESP8266)
    Serial.printf("board:%s\n", "ESP8266");
#endif
    Serial.printf("MAC:%s\n", MAC.c_str());
    Serial.printf("gpio:%d\n", gpioInUse);
    Serial.printf("baudrate:%d\n", baudRateInUse);
    Serial.printf("effect:%d\n", effect);
    Serial.printf("colorMode:%d\n", colorMode);

  }

}