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



class LedManager {




private:
//    RgbwColor calculateRgbwMode(uint8_t r, uint8_t g, uint8_t b);
//    RgbColor calculateRgbMode(uint8_t r, uint8_t g, uint8_t b);


public:
    /**
 * Dynamic PIN Template
 */
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
    const String COLOR_MODE_FILENAME = "color_mode.json";
    const String COLOR_MODE_PARAM = "colorMode";
    uint16_t dynamicLedNum = 511;

    void cleanLEDs();
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
    void setTemperature(int whitetemp);

};

#endif //GLOW_WORM_LUCIFERIN_LEDMANAGER_H
