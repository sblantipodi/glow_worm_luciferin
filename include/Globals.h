/*
  Globals.h - Helper classes
  
  Copyright © 2020 - 2024  Davide Perini
  
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
#define SERIAL_SIZE_RX 2048
#define CONFIG_NUM_PARAMS 20
#define CONFIG_PREFIX_LENGTH 6
// This value must meet the one in Firefly Luciferin
// We are transferring byte via Serial, the maximum decimal number that can be represented with 1 byte is 255.
// Use a multiplier to set a much bigger number using only 2 bytes.
const int SERIAL_CHUNK_SIZE = 250;

extern class BootstrapManager bootstrapManager;

extern class EffectsManager effectsManager;

extern class LedManager ledManager;

extern class NetworkManager networkManager;

extern class Helpers helper;

extern class Globals globals;

// Change this number if you increase/decrease the usb serial config variables
extern byte config[CONFIG_NUM_PARAMS];
extern byte pre[CONFIG_PREFIX_LENGTH];
extern uint8_t prefix[], hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp, fireflyEffect,
        fireflyColorMode, fireflyColorOrder, ldrEn, ldrTo, ldrInt, ldrMn, ldrAction, relaySerialPin, sbSerialPin, ldrSerialPin, gpioClock;

extern uint8_t gpioInUse;
extern uint8_t gpioClockInUse;
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
extern float framerateSerial;
extern float framerateCounter;
extern float framerateCounterSerial;
extern uint lastStream;
const String GPIO_PARAM = "gpio";
const String GPIO_CLOCK_PARAM = "gpioClock";
const String GPIO_FILENAME = "gpio.json";
const String GPIO_CLOCK_FILENAME = "gpioClock.json";
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
extern unsigned long lastUdpMsgReceived;

extern unsigned long currentMillisCheckConn;
extern unsigned long prevMillisCheckConn1;
extern unsigned long prevMillisCheckConn2;

extern unsigned long currentMillisSendSerial;
extern unsigned long prevMillisSendSerial;

extern unsigned long currentMillisMainLoop;

extern unsigned long prevMillisPing;

extern uint8_t baudRateInUse;
extern bool relayState;
extern bool breakLoop;
extern bool apFileRead;
extern int disconnectionCounter;

class Globals {

public:
    static void setGpio(int gpio);

    static void setGpioClock(int gpioClock);

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

