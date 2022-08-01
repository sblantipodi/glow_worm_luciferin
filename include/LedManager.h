/*
  LedManager.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright (C) 2020 - 2022  Davide Perini

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

#ifndef GLOW_WORM_LUCIFERIN_LEDMANAGER_H
#define GLOW_WORM_LUCIFERIN_LEDMANAGER_H

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "Globals.h"

const String LED_NUM_PARAM = "lednum";
const String LED_NUM_FILENAME = "led_number.json";
const String WHITE_TEMP_PARAM = "whitetemp";
const String WHITE_TEMP_FILENAME = "whitetemp.json";
const uint8_t WHITE_TEMP_CORRECTION_DISABLE = 65;

class LedManager {

  public:

#if defined(ESP32)
      NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod>* ledsEsp32 = NULL; // Hardware, ALL GPIO, yes serial read/write
      NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Ws2812xMethod>* ledsEsp32Rgbw = NULL; // Hardware, ALL GPIO, yes serial read/write
#else
      NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> *ledsDma = NULL; // Hardware DMA, GPIO3, no serial read, yes serial write
      NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> *ledsDmaRgbw = NULL; // Hardware DMA, GPIO3, no serial read, yes serial write
      NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> *ledsUart = NULL; // Hardware UART, GPIO2, yes serial read/write
      NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart1800KbpsMethod> *ledsUartRgbw = NULL; // Hardware UART, GPIO2, yes serial read/write
      NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBangWs2812xMethod> *ledsStandard = NULL; // No hardware, ALL GPIO, yes serial read/write
      NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBangWs2812xMethod> *ledsStandardRgbw = NULL; // No hardware, ALL GPIO, yes serial read/write
#endif
      CRGB leds[511];
      const String COLOR_MODE_FILENAME = "color_mode.json";
      const String COLOR_MODE_PARAM = "colorMode";
      const String LDR_FILENAME = "ldr.json";
      const String LDR_CAL_FILENAME = "ldrCal.json";
      const String LDR_PARAM = "ldr";
      const String LDR_CONT_PARAM = "ldrCont";
      const String MIN_LDR_PARAM = "minLdr";
      const String MAX_LDR_PARAM = "maxLdr";
      const String EFFECT_FILENAME = "effect.json";
      uint16_t dynamicLedNum = 511;
      byte red = 255;
      byte green = 255;
      byte blue = 255;
      bool stateOn = false;
      boolean reinitLEDTriggered = false;
      uint lastLedUpdate = 10000;
      bool led_state = true;
      uint8_t fireflyEffectInUse;

      void cleanLEDs();
      void initEsp32();
      void initEsp32Rgbw();
      void initLeds();
      void initStandard();
      void initStandardRgbw();
      void initUart();
      void initUartRgbw();
      void initDma();
      void initDmaRgbw();
      void ledShow();
      void setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
      void setColorMode(int colorModeToUse);
      void setColorModeInit(uint8_t newColorMode);
      void setColor(uint8_t inR, uint8_t inG, uint8_t inB);
      void setNumLed(int numLedFromLuciferin);
      void setWhiteTemp(int whiteTemp);
      void setLdr(boolean ldrEnabled, boolean ldrContinuous, String minLdr);
      void setLdr(int maxLdr);

};

#endif //GLOW_WORM_LUCIFERIN_LEDMANAGER_H
