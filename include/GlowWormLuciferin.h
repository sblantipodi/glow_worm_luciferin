/*
  GlowWormLuciferin.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2025  Davide Perini

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#if defined(ARDUINO_ARCH_ESP32)
//#define FASTLED_INTERRUPT_RETRY_COUNT 0
//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP32_I2S true
#endif

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "Version.h"
#include "Globals.h"
#if defined(ESP8266)
#include "PingESP.h"
#endif

#if defined(ARDUINO_ARCH_ESP32)
TaskHandle_t handleTcpTask = NULL; // fast TCP task pinned to CORE0
TaskHandle_t handleSerialTask = NULL; // fast Serial task pinned to CORE1
#elif defined(ESP8266)
PingESP pingESP;
#endif

// This must be a multiple of 3 (R;G;B). Serial buffer is read with a single block using Serial.readBytes(),
// if there are many LEDs and buffer is too small, read the first block with Serial.readBytes() and then continue with Serial.read()
const uint16_t LED_BUFF = 1500;
byte ledBuffer[LED_BUFF];
int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin
#if defined(ARDUINO_ARCH_ESP32)
unsigned long esp32DebouceInitialPeriod = 30000;
#else
unsigned long esp8266DebouceInitialPeriod = 60000;
#endif
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


void mainLoop();

void manageApRoot();

void setApState(byte state);

void configureLeds();

void setSerialPixel(int j, byte r, byte g, byte b);

void debounceSmartButton();
