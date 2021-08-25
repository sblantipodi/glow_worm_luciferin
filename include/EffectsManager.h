/*
  EffectsManager.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright (C) 2020 - 2021  Davide Perini

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

#ifndef GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
#define GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H

#include <Arduino.h>
#include <FastLED.h>

class EffectsManager {

#define NUM_LEDS 511 // Max Led support

private:

    CRGB scroll(int pos);
    byte red = 255;
    byte green = 255;
    byte blue = 255;
    long lastAnim = 0;
    int mixedRainboxIndex = 0;
    uint16_t maxChanges = 48;
    uint16_t gHue = 0;
    uint16_t thishue = 0;
    uint16_t deltahue = 10;
    long lastAnimSolidRainbow = 0;

public:

    void fire(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
              int cooling, int sparking, int speedDelay, int dynamicLedNum);

    void twinkleRandom(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
                       void (*setColor)(uint8_t r, uint8_t g, uint8_t b), int count, int speedDelay, boolean onlyOne,
                       int dynamicLedNum);

    void theaterChaseRainbow(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
                        int dynamicLedNum);

    void mixedRainbow(void (*ledShowCallback)(), void (*checkConnectionCallback)(),
                      void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b), CRGB leds[NUM_LEDS],
                      int dynamicLedNum);

    void bpm(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
             CRGB leds[NUM_LEDS], CRGBPalette16 currentPalette, CRGBPalette16 targetPalette);

    void rainbow(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
                 CRGB leds[NUM_LEDS], int dynamicLedNum);

    void solidRainbow(void (*ledShowCallback)(), void (*setPixelColorCallback)(uint16_t, uint8_t r, uint8_t g, uint8_t b),
                      CRGB leds[NUM_LEDS], int dynamicLedNum);

};

#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
