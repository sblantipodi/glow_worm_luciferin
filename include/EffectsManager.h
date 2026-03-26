/*
  EffectsManager.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2026  Davide Perini

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

    static void twinkleRandom();

    void theaterChaseRainbow();

    void mixedRainbow();

    void bpm();

    void colorWipe(byte red, byte green, byte blue);

    static RgbColor Wheel(uint8_t WheelPos);

    void solidRainbow();

    static void randomColors();

    static void rainbowColors();

    static void meteor();

    static void colorWaterfall();

    static void randomMarquee();

    static void rainbowMarquee();

    static void pulsing_rainbow();

    static void christmas();

    void fire(int cooling, int sparking, int speedDelay);

    void rainbow(boolean slowdown);
};

#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
