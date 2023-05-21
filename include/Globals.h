/*
  Globals.h - Helper classes
  
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

#ifndef _DPSOFTWARE_GLOBALS_UTILS_H
#define _DPSOFTWARE_GLOBALS_UTILS_H

#include <Arduino.h>
#include <lib8tion.h>
#include "BootstrapManager.h"
#include "EffectsManager.h"
#include "LedManager.h"
#include "NetworkManager.h"

#if defined(ESP8266)
#define LDR_DIVIDER 1024
#endif
#if defined(ARDUINO_ARCH_ESP32)
#define LDR_DIVIDER 4096
#endif
#define SERIAL_SIZE_RX  2048

extern class BootstrapManager bootstrapManager;

extern class EffectsManager effectsManager;

extern class LedManager ledManager;

extern class NetworkManager networkManager;

extern class Helpers helper;

extern class Globals globals;

extern uint8_t prefix[], hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp, fireflyEffect,
        fireflyColorMode, fireflyColorOrder, ldrEn, ldrTo, ldrInt, ldrMn, ldrAction, relaySerialPin, sbSerialPin, ldrSerialPin;
extern uint8_t prefixLength;

extern uint8_t gpioInUse;
extern uint8_t whiteTempInUse;
extern uint8_t colorMode;
extern uint8_t colorOrder;
extern byte brightness;
extern byte rStored;
extern byte gStored;
extern byte bStored;
extern byte brightnessStored;
extern boolean autoSave;
enum class Effect {
    GlowWormWifi, GlowWorm, solid, fire, twinkle, bpm, rainbow, chase_rainbow, solid_rainbow, mixed_rainbow
};
extern Effect effect;
extern String ffeffect;
extern float framerate;
extern float framerateCounter;
extern uint lastStream;
const String GPIO_PARAM = "gpio";
const String GPIO_FILENAME = "gpio.json";
const String AUTO_SAVE_FILENAME = "as.json";
const String COLOR_BRIGHT_FILENAME = "cb.json";
const String AP_FILENAME = "ap.json";
const String BAUDRATE_PARAM = "baudrate";
const String AP_PARAM = "ap";
const String BAUDRATE_FILENAME = "baudrate.json";
extern bool ldrReading;
extern int ldrValue;
extern bool ldrEnabled;
extern bool ledOn;
extern uint8_t ldrInterval;
extern bool ldrTurnOff;
extern uint8_t ldrMin;
extern uint8_t relayPin;
extern uint8_t sbPin;
extern uint8_t ldrPin;
extern int ldrDivider;
extern const unsigned int LDR_RECOVER_TIME;
extern unsigned long previousMillisLDR;

extern uint8_t baudRateInUse;
extern bool relayState;
extern bool breakLoop;
extern bool apFileRead;
extern int disconnectionCounter;

class Globals {

public:
    static void setGpio(int gpio);

    static void saveColorBrightnessInfo(int r, int g, int b, int brightness);

    static void setBaudRate(int bdRate);

    static int setBaudRateInUse(int bdrate);

    static void turnOffRelay();

    static void turnOnRelay();

    static void sendSerialInfo();

    static const char *effectToString(Effect e);

    static const uint8_t effectToInt(Effect e);

};


#endif

