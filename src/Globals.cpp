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
byte config[CONFIG_NUM_PARAMS];
byte pre[CONFIG_PREFIX_LENGTH];
uint8_t prefix[] = {'D', 'P', 's', 'o', 'f', 't'}, hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp,
        fireflyEffect, fireflyColorMode, fireflyColorOrder, ldrEn, ldrTo, ldrInt, ldrMn, ldrAction, relaySerialPin, sbSerialPin, ldrSerialPin, gpioClock;
uint8_t whiteTempInUse = WHITE_TEMP_CORRECTION_DISABLE;
uint8_t colorMode = 1;
uint8_t colorOrder = 1;
byte brightness = 255;
byte rStored;
byte gStored;
byte bStored;
byte brightnessStored;
boolean autoSave;
Effect effect;
String ffeffect;
float framerate = 0;
float framerateSerial = 0;
float framerateCounter = 0;
float framerateCounterSerial = 0;
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
#if defined(ESP8266)
uint8_t gpioInUse = 2;
uint8_t relayPin = 12;
uint8_t sbPin = 0;
uint8_t ldrPin = A0;
#endif
#if CONFIG_IDF_TARGET_ESP32C3
uint8_t gpioInUse = 6;
uint8_t relayPin = 0;
uint8_t sbPin = 9;
uint8_t ldrPin = 3;
#elif CONFIG_IDF_TARGET_ESP32S2
uint8_t gpioInUse = 16;
uint8_t relayPin = 9;
uint8_t sbPin = 0;
uint8_t ldrPin = 3;
#elif CONFIG_IDF_TARGET_ESP32S3
uint8_t gpioInUse = 16;
uint8_t relayPin = 13;
uint8_t sbPin = 0;
uint8_t ldrPin = 2;
#elif CONFIG_IDF_TARGET_ESP32
uint8_t gpioInUse = 2;
uint8_t relayPin = 12; // 22 for PICO
uint8_t sbPin = 0;
uint8_t ldrPin = 36; // 33 for PICO
#endif
uint8_t gpioClockInUse = 2;

bool ledOn = false;
int ldrDivider = LDR_DIVIDER;
const unsigned int LDR_RECOVER_TIME = 4000;
unsigned long previousMillisLDR = 0;
unsigned long lastUdpMsgReceived;
bool apFileRead;
int disconnectionCounter;

unsigned long currentMillisCheckConn = 0;
unsigned long prevMillisCheckConn1 = 0;
unsigned long prevMillisCheckConn2 = 0;

unsigned long currentMillisSendSerial = 0;
unsigned long prevMillisSendSerial = 0;

unsigned long currentMillisMainLoop = 0;

unsigned long prevMillisPing = 0;

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
 * Set gpio clock received by the Firefly Luciferin software
 * @param gpio gpio to use
 */
void Globals::setGpioClock(int gpioClockToUse) {
  Serial.println("CHANGING GPIO CLOCK");
  if (gpioClockToUse == 0) {
    gpioClockToUse = 2;
  }
  gpioClockInUse = gpioClockToUse;
  DynamicJsonDocument gpioClockDoc(1024);
  gpioClockDoc[GPIO_CLOCK_PARAM] = gpioClockInUse;
  BootstrapManager::writeToLittleFS(gpioClockDoc, GPIO_CLOCK_FILENAME);
  delay(20);
}

/**
 * Store color and brightness info
 * @param gpio gpio to use
 */
void Globals::saveColorBrightnessInfo(int r, int g, int b, int brightness) {
  Serial.println(F("Saving color and brightness info"));
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[F("r")] = rStored = r;
  gpioDoc[F("g")] = gStored = g;
  gpioDoc[F("b")] = bStored = b;
  gpioDoc[F("brightness")] = brightnessStored = brightness;
  BootstrapManager::writeToLittleFS(gpioDoc, COLOR_BRIGHT_FILENAME);
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
    case 1:
      baudRateToUse = 230400;
      break;
    case 2:
      baudRateToUse = 460800;
      break;
    case 4:
      baudRateToUse = 921600;
      break;
    case 5:
      baudRateToUse = 1000000;
      break;
    case 6:
      baudRateToUse = 1500000;
      break;
    case 7:
      baudRateToUse = 2000000;
      break;
    case 8:
      baudRateToUse = 115200;
      break;
    default:
      baudRateToUse = 500000;
      break;
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
    digitalWrite(relayPin, HIGH);
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
    digitalWrite(relayPin, LOW);
  }
}

/**
 * Send serial info
 */
void Globals::sendSerialInfo() {
  currentMillisSendSerial = millis();
  if (currentMillisSendSerial - prevMillisSendSerial > 10000) {
    prevMillisSendSerial = currentMillisSendSerial;
    if (currentMillisSendSerial > lastUdpMsgReceived + DELAY_1000) {
      framerateSerial = framerateCounterSerial > 0 ? framerateCounterSerial / 10 : 0;
      framerateCounterSerial = 0;
      Serial.printf("framerate:%s\n", (String((framerateSerial > 0.5 ? framerateSerial : 0),1)).c_str());
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
      Serial.printf("firmware:%s\n", "LIGHT");
#else
      Serial.printf("firmware:%s\n", "FULL");
      Serial.printf("mqttopic:%s\n", networkManager.topicInUse.c_str());
#endif
      Serial.printf("ver:%s\n", VERSION);
      Serial.printf("lednum:%d\n", ledManager.dynamicLedNum);
#if defined(ESP8266)
      Serial.printf("board:%s\n", "ESP8266");
#endif
#if CONFIG_IDF_TARGET_ESP32C3
      Serial.printf("board:%s\n", "ESP32_C3_CDC");
#elif CONFIG_IDF_TARGET_ESP32S2
      Serial.printf("board:%s\n", "ESP32_S2");
#elif CONFIG_IDF_TARGET_ESP32S3
#if ARDUINO_USB_MODE==1
      Serial.printf("board:%s\n", "ESP32_S3_CDC");
#else
      Serial.printf("board:%s\n", "ESP32_S3");
#endif
#elif CONFIG_IDF_TARGET_ESP32
      Serial.printf("board:%s\n", "ESP32");
#endif
      Serial.printf("MAC:%s\n", MAC.c_str());
      Serial.printf("gpio:%d\n", gpioInUse);
      Serial.printf("gpioClock:%d\n", gpioClockInUse);
      Serial.printf("baudrate:%d\n", baudRateInUse);
      Serial.printf("effect:%d\n", Globals::effectToInt(effect));
      Serial.printf("colorMode:%d\n", colorMode);
      Serial.printf("colorOrder:%d\n", colorOrder);
      Serial.printf("white:%d\n", whiteTempInUse);
      if (ldrEnabled) {
        Serial.printf("ldr:%d\n", ((ldrValue * 100) / ldrDivider));
      }
      Serial.printf("relayPin:%d\n", relayPin);
      Serial.printf("sbPin:%d\n", sbPin);
      Serial.printf("ldrPin:%d\n", ldrPin);
    }
  }
}

/**
 * Return effect string
 * @param e effect enum
 * @return  effect string
 */
const char *Globals::effectToString(Effect e) {
  switch (e) {
    case Effect::bpm:
      return "Bpm";
    case Effect::fire:
      return "Fire";
    case Effect::twinkle:
      return "Twinkle";
    case Effect::rainbow:
      return "Rainbow";
    case Effect::chase_rainbow:
      return "Chase rainbow";
    case Effect::solid_rainbow:
      return "Solid rainbow";
    case Effect::mixed_rainbow:
      return "Mixed rainbow";
    case Effect::GlowWorm:
      return "GlowWorm";
    case Effect::GlowWormWifi:
      return "GlowWormWifi";
    default:
      return "Solid";
  }
}

const uint8_t Globals::effectToInt(Effect e) {
  switch (e) {
    case Effect::bpm:
      return 5;
    case Effect::fire:
      return 3;
    case Effect::twinkle:
      return 4;
    case Effect::rainbow:
      return 6;
    case Effect::chase_rainbow:
      return 7;
    case Effect::solid_rainbow:
      return 8;
    case Effect::mixed_rainbow:
      return 9;
    case Effect::GlowWorm:
      return 1;
    case Effect::GlowWormWifi:
      return 0;
    default:
      return 2;
  }
}
