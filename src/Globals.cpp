/*
  Globals.cpp - Helper classes
  
  Copyright © 2020 - 2026  Davide Perini
  
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
#include "NetManager.h"

BootstrapManager bootstrapManager;
EffectsManager effectsManager;
LedManager ledManager;
NetManager netManager;
// DPsoftware checksum for serial
byte config[CONFIG_NUM_PARAMS];
byte pre[CONFIG_PREFIX_LENGTH];
uint8_t prefix[] = {'D', 'P', 's', 'o', 'f', 't'}, hi, lo, chk, usbBrightness, gpio, baudRate, whiteTemp,
        fireflyEffect, fireflyColorMode, fireflyColorOrder, ldrEn, ldrTo, ldrInt, ldrMn, ldrAction, relaySerialPin, relayInvPin, sbSerialPin, ldrSerialPin, gpioClock;
uint8_t whiteTempInUse = WHITE_TEMP_CORRECTION_DISABLE;
uint8_t colorMode = 1;
uint8_t colorOrder = 1;
byte brightness = 255;
byte rStored;
byte gStored;
byte bStored;
byte brightnessStored;
Effect effectStored;
boolean toggleStored;
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
volatile int ldrValue;
bool ldrEnabled = false;
bool relInv = false;
uint8_t ldrInterval = 30;
bool ldrTurnOff = false;
uint8_t ldrMin = 20;
#if defined(ESP8266)
uint8_t gpioInUse = 2;
uint8_t relayPin = 12;
uint8_t sbPin = 0;
uint8_t ldrPin = A0;
int8_t ledBuiltin = -1;
#endif
#if CONFIG_IDF_TARGET_ESP32C3
uint8_t gpioInUse = 6;
uint8_t relayPin = 0;
uint8_t sbPin = 9;
uint8_t ldrPin = 3;
int8_t ledBuiltin = 7;
#elif CONFIG_IDF_TARGET_ESP32S2
uint8_t gpioInUse = 16;
uint8_t relayPin = 9;
uint8_t sbPin = 0;
uint8_t ldrPin = 3;
int8_t ledBuiltin = 15;
#elif CONFIG_IDF_TARGET_ESP32S3
uint8_t gpioInUse = 16;
uint8_t relayPin = 13;
uint8_t sbPin = 0;
uint8_t ldrPin = 2;
int8_t ledBuiltin = 47;
#elif CONFIG_IDF_TARGET_ESP32C6
uint8_t gpioInUse = 5;
uint8_t relayPin = 15;
uint8_t sbPin = 1;
uint8_t ldrPin = 2;
int8_t ledBuiltin = 8;
#elif CONFIG_IDF_TARGET_ESP32C5
uint8_t gpioInUse = 0;
uint8_t relayPin = 14; // don't use 15
uint8_t sbPin = 1;
uint8_t ldrPin = 6;
int8_t ledBuiltin = 27;
#elif CONFIG_IDF_TARGET_ESP32
uint8_t gpioInUse = 2;
uint8_t relayPin = 12;
uint8_t sbPin = 0;
uint8_t ldrPin = 36;
int8_t ledBuiltin = -1;
#endif
uint8_t gpioClockInUse = 2;

bool ledOn = false;
int ldrDivider = LDR_DIVIDER;
const unsigned int LDR_RECOVER_TIME = 4000;
unsigned long previousMillisLDR = 0;
unsigned long lastUdpMsgReceived;
unsigned long disconnectionTime;
// after three minutes, start device reset
unsigned long secondsBeforeReset = 180000;
bool apFileRead;

unsigned long currentMillisCheckConn = 0;
unsigned long prevMillisCheckConn1 = 0;
unsigned long prevMillisCheckConn2 = 0;

unsigned long currentMillisSendSerial = 0;
unsigned long prevMillisSendSerial = 0;
unsigned long prevMillisPing = 0;

/**
 * Set gpio received by the Firefly Luciferin software
 * @param gpio gpio to use
 */
void Globals::setGpio(int gpioToUse) {
  Serial.println("CHANGING GPIO");
  gpioInUse = gpioToUse;
  JsonDocument gpioDoc;
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
  gpioClockInUse = gpioClockToUse;
  JsonDocument gpioClockDoc;
  gpioClockDoc[GPIO_CLOCK_PARAM] = gpioClockInUse;
  BootstrapManager::writeToLittleFS(gpioClockDoc, GPIO_CLOCK_FILENAME);
  delay(20);
}

/**
 * Store color and brightness info
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @param brightness stored
 * @param requestedEffect stored
 * @param toggleStored stored
 */
void Globals::saveColorBrightnessInfo(int r, int g, int b, int bright, String requestedEffect, boolean toggle) {
  Serial.println(F("Saving color and brightness info"));
  JsonDocument gpioDoc;
  gpioDoc[F("r")] = rStored = r;
  gpioDoc[F("g")] = gStored = g;
  gpioDoc[F("b")] = bStored = b;
  gpioDoc[F("toggle")] = toggleStored = toggle;
  gpioDoc[F("brightness")] = brightnessStored = bright;
  if (requestedEffect == "GlowWormWifi" || requestedEffect == "GlowWorm") {
    requestedEffect = "Solid";
  }
  gpioDoc[F("effect")] = requestedEffect;
  effectStored = Globals::stringToEffect(requestedEffect);
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
    case 9:
      baudRateToUse = 4000000;
      break;
    case 10:
      baudRateToUse = 6000000;
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
  JsonDocument baudrateDoc;
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
    digitalWrite(relayPin, relInv ? LOW : HIGH);
    delay(10);
  }
}

/**
 * Turn OFF the relay
 */
void Globals::turnOffRelay() {
  if (relayState) {
    relayState = false;
    delay(10);
    digitalWrite(relayPin, relInv ? HIGH : LOW);
  }
}

/**
 * Return the LDR reading as a percentage, 0 if the divider is not valid
 * @return ldr percentage (0..100)
 */
int Globals::ldrPercent() {
  return ldrDivider > 0 ? (ldrValue * 100) / ldrDivider : 0;
}

/**
 * Return the board name
 * @return board name string
 */
const char *Globals::boardName() {
#if defined(ESP8266)
  return "ESP8266";
#elif CONFIG_IDF_TARGET_ESP32C3
  return "ESP32_C3";
#elif CONFIG_IDF_TARGET_ESP32C6
  return "ESP32_C6";
#elif CONFIG_IDF_TARGET_ESP32C5
  return "ESP32_C5";
#elif CONFIG_IDF_TARGET_ESP32S2
  return "ESP32_S2";
#elif CONFIG_IDF_TARGET_ESP32S3
  return "ESP32_S3";
#elif CONFIG_IDF_TARGET_ESP32
  return "ESP32";
#else
  return "UNKNOWN";
#endif
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
        Serial.printf("framerate:%f\r\n", (framerateSerial > 0.5 ? framerateSerial : 0));
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
        Serial.printf("firmware:%s\r\n", "LIGHT");
#else
        Serial.printf("firmware:%s\r\n", "FULL");
        Serial.printf("mqttopic:%s\r\n", netManager.topicInUse.c_str());
#endif
        Serial.printf("ver:%s\r\n", VERSION);
        Serial.printf("lednum:%d\r\n", ledManager.dynamicLedNum);
        Serial.printf("board:%s\r\n", Globals::boardName());
        Serial.printf("MAC:%s\r\n", MAC.c_str());
        Serial.printf("gpio:%d\r\n", gpioInUse);
        Serial.printf("gpioClock:%d\r\n", gpioClockInUse);
        Serial.printf("baudrate:%d\r\n", baudRateInUse);
        Serial.printf("effect:%d\r\n", Globals::effectToInt(effect));
        Serial.printf("colorMode:%d\r\n", colorMode);
        Serial.printf("colorOrder:%d\r\n", colorOrder);
        Serial.printf("white:%d\r\n", whiteTempInUse);
        if (ldrEnabled) {
          Serial.printf("ldr:%d\r\n", ldrPercent());
      }
      Serial.printf("relayPin:%d\r\n", relayPin);
      Serial.printf("relInv:%d\r\n", relInv);
      Serial.printf("sbPin:%d\r\n", sbPin);
      Serial.printf("ldrPin:%d\r\n", ldrPin);
    }
  }
}

/**
 * Effect table: index = Effect enum value, code = protocol value.
 * Keep in sync with the Effect enum order in Globals.h.
 */
struct EffectDef {
  const char *name;
  uint8_t code;
};
static const EffectDef kEffects[] = {
  { "GlowWormWifi", 0 },    // Effect::GlowWormWifi
  { "GlowWorm", 1 },        // Effect::GlowWorm
  { "Solid", 2 },           // Effect::solid
  { "Fire", 3 },            // Effect::fire
  { "Twinkle", 4 },         // Effect::twinkle
  { "Bpm", 5 },             // Effect::bpm
  { "Rainbow", 6 },         // Effect::rainbow
  { "Chase rainbow", 7 },   // Effect::chase_rainbow
  { "Solid rainbow", 8 },   // Effect::solid_rainbow
  { "Slow rainbow", 9 },    // Effect::slowRainbow
  { "Random colors", 10 },  // Effect::randomColors
  { "Rainbow colors", 11 }, // Effect::rainbowColors
  { "Meteor", 12 },         // Effect::meteor
  { "Color waterfall", 13 },// Effect::colorWaterfall
  { "Random marquee", 14 }, // Effect::randomMarquee
  { "Rainbow marquee", 15 },// Effect::rainbowMarquee
  { "Pulsing rainbow", 16 },// Effect::pulsing_rainbow
  { "Christmas", 17 }       // Effect::christmas
};
static const size_t NUM_EFFECTS = sizeof(kEffects) / sizeof(kEffects[0]);

/**
 * Return effect string
 * @param e effect enum
 * @return  effect string
 */
const char *Globals::effectToString(Effect e) {
  int idx = static_cast<int>(e);
  return (idx >= 0 && idx < static_cast<int>(NUM_EFFECTS)) ? kEffects[idx].name : "Solid";
}

/**
 * String to effect
 * @param requestedEffect string
 * @return effect
 */
Effect Globals::stringToEffect(String requestedEffect) {
  for (size_t i = 0; i < NUM_EFFECTS; i++) {
    if (requestedEffect == kEffects[i].name) {
      return static_cast<Effect>(i);
    }
  }
  return Effect::solid;
}

const uint8_t Globals::effectToInt(Effect e) {
  int idx = static_cast<int>(e);
  return (idx >= 0 && idx < static_cast<int>(NUM_EFFECTS)) ? kEffects[idx].code : 2;
}
