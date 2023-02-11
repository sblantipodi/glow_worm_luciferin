/*
  Globals.cpp - Helper classes
  
  Copyright © 2020 - 2023  Davide Perini
  
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

// DPsoftware checksum for serial
uint8_t prefix[] = {'D', 'P', 's', 'o', 'f', 't'}, hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp,
                    fireflyEffect, fireflyColorMode, ldrEn, ldrTo, ldrInt, ldrMn, ldrAction, prefixLength = 6;
uint8_t gpioInUse = 2;
uint8_t whiteTempInUse = WHITE_TEMP_CORRECTION_DISABLE;
uint8_t colorMode = 1;
byte brightness = 255;
Effect effect;
String ffeffect;
float framerate = 0;
float framerateCounter = 0;
uint lastStream = 0;
#ifdef TARGET_GLOWWORMLUCIFERINFULL
uint8_t baudRateInUse = 8;
#else
uint8_t baudRateInUse = 3;
#endif
bool relayState = false;
bool breakLoop = false;
bool ldrReading = false;
int ldrValue;
bool ldrEnabled = false;
uint8_t ldrInterval = 30;
bool ldrTurnOff = false;
uint8_t ldrMin = 20;
int ldrDivider = LDR_DIVIDER;
const unsigned int LDR_RECOVER_TIME = 4000;
unsigned long previousMillisLDR = 0;

/**
 * Set gpio received by the Firefly Luciferin software
 * @param gpio gpio to use
 */
void Globals::setGpio(int gpioToUse) {

  Serial.println("CHANGING GPIO");
  if (gpioToUse == 0) {
    gpioToUse = 2;
  }
  gpioInUse = gpioToUse;
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[GPIO_PARAM] = gpioInUse;
  BootstrapManager::writeToLittleFS(gpioDoc, GPIO_FILENAME);
  delay(20);

}

/**
 * Set the baudrate on the microcontroller
 * @param bdrate supported baud rate
 * @return baudrate index
 */
int Globals::setBaudRateInUse(int bdrate) {

  baudRateInUse = bdrate;
  int baudRateToUse;
  switch (bdrate) {
    case 1: baudRateToUse = 230400; break;
    case 2: baudRateToUse = 460800; break;
    case 4: baudRateToUse = 921600; break;
    case 5: baudRateToUse = 1000000; break;
    case 6: baudRateToUse = 1500000; break;
    case 7: baudRateToUse = 2000000; break;
    case 8: baudRateToUse = 115200; break;
    default: baudRateToUse = 500000; break;
  }
  return baudRateToUse;

}

/**
 * Set bdRate received by the Firefly Luciferin software
 * @param bdRate int
 */
void Globals::setBaudRate(int bdRate) {

  Serial.println(F("CHANGING BAUDRATE"));
  setBaudRateInUse(bdRate);
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
  BootstrapManager::writeToLittleFS(baudrateDoc, BAUDRATE_FILENAME);
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
    Serial.printf("white:%d\n", whiteTempInUse);
    if (ldrEnabled) {
      Serial.printf("ldr:%d\n", ((ldrValue * 100) / ldrDivider));
    }
  }

}

/**
 * Return effect string
 * @param e effect enum
 * @return  effect string
 */
const char* Globals::effectToString(Effect e) {

  switch (e) {
    case Effect::bpm: return "Bpm";
    case Effect::fire: return "Fire";
    case Effect::twinkle: return "Twinkle";
    case Effect::rainbow: return "Rainbow";
    case Effect::chase_rainbow: return "Chase rainbow";
    case Effect::solid_rainbow: return "Solid rainbow";
    case Effect::mixed_rainbow: return "Mixed rainbow";
    case Effect::GlowWorm: return "GlowWorm";
    case Effect::GlowWormWifi: return "GlowWormWifi";
    default: return "Solid";
  }

}
