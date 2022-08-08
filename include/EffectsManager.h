/*
  EffectsManager.h - Glow Worm Luciferin for Firefly Luciferin
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

#ifndef GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
#define GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include "LedManager.h"
#include "Globals.h"

const int NUM_LEDS = 511;

class EffectsManager {

  private:

      static CRGB scroll(int pos);
      byte red = 255;
      byte green = 255;
      byte blue = 255;
      unsigned long lastAnim = 0;
      int mixedRainboxIndex = 0;
      uint16_t maxChanges = 48;
      uint16_t gHue = 0;
      uint16_t thishue = 0;
      uint16_t deltahue = 10;
      unsigned long lastAnimSolidRainbow = 0;

  public:

      static void fire(int cooling, int sparking, int speedDelay, int dynamicLedNum);
      static void twinkleRandom(int count, int speedDelay, boolean onlyOne, int dynamicLedNum);
      static void theaterChaseRainbow(int dynamicLedNum);
      void mixedRainbow(int dynamicLedNum);
      void bpm(CRGBPalette16 currentPalette, CRGBPalette16 targetPalette);
      void rainbow(int dynamicLedNum);
      void solidRainbow(int dynamicLedNum);

};

#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
