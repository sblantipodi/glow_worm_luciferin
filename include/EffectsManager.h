/*
  EffectsManager.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2024  Davide Perini

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
#include "LedManager.h"
#include "Globals.h"

const int NUM_LEDS = 511;

class EffectsManager {

private:

    unsigned long lastAnim = 0;

public:

    static void fire(int cooling, int sparking, int speedDelay, int dynamicLedNum);

    static void twinkleRandom(int count, int speedDelay, boolean onlyOne, int dynamicLedNum);

    void theaterChaseRainbow(int dynamicLedNum);

    void mixedRainbow(int dynamicLedNum);

    void bpm(int dynamicLedNum);

    void rainbow(int dynamicLedNum);

    void solidRainbow(int dynamicLedNum);

    void colorWipe(int dynamicLedNum, byte red, byte green, byte blue);

    static RgbColor Wheel(uint8_t WheelPos);

};

#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
