/*
  Helpers.h - Helper classes
  
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

#ifndef _DPSOFTWARE_GLOBALS_UTILS_H
#define _DPSOFTWARE_GLOBALS_UTILS_H

#include <Arduino.h>
#include "BootstrapManager.h"
#include "EffectsManager.h"
#include "LedManager.h"
#include "NetworkManager.h"

#if defined(ESP32)
#define RELAY_PIN_DIG 23 // equals to Q4
#define RELAY_PIN_PICO 22
#elif defined(ESP8266)
#define RELAY_PIN 12
#endif

extern class BootstrapManager bootstrapManager;
extern class EffectsManager effectsManager;
extern class LedManager ledManager;
extern class NetworkManager networkManager;
extern class Helpers helper;
extern class Globals globals;

extern uint8_t prefix[], hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp, fireflyEffect, fireflyColorMode, i;
extern uint8_t prefixLength;

extern uint8_t gpioInUse;
extern uint8_t colorMode;
extern byte brightness;
extern uint8_t whiteTempCorrection[];
enum class Effect { GlowWormWifi, GlowWorm, solid, fire, twinkle, bpm, rainbow, chase_rainbow, solid_rainbow, mixed_rainbow };
extern Effect effect;
extern float framerate;
extern float framerateCounter;
extern uint lastStream;
const String GPIO_PARAM = "gpio";
const String GPIO_FILENAME = "gpio.json";
const String BAUDRATE_PARAM = "baudrate";
const String BAUDRATE_FILENAME = "baudrate.json";

extern uint8_t baudRateInUse;
extern bool relayState;
extern bool breakLoop;

class Globals {

public:
    void setGpio(int gpio);
    void setBaudRate(int baudRate);
    int setBaudRateInUse(int baudRate);
    void turnOffRelay();
    void turnOnRelay();
    void sendSerialInfo();

};


#endif

