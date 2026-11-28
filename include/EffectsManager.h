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

class EffectsManager {

private:

    unsigned long lastAnim = 0;
    byte* heat = nullptr;
    int heatSize = 0;

    // Effect state
    RgbColor color = Wheel(random(0, 255));
    unsigned long preMill = 0;
    int position = 0;
    uint8_t hue = 0;
    int currentPixel = 0;
    uint16_t iWipe = 0;
    uint16_t jMixed = 0;
    uint16_t mixed = 0;
    int kFade = 0;
    bool stepFadeIn = true;
    byte *cT = nullptr;
    uint16_t iT = 0;
    uint16_t jT = 0;
    int xSolidRainbow = 0;
    int ySolidRainbow = 0;

    static void setAll(byte red, byte green, byte blue);
    void FadeInOut(byte red, byte green, byte blue);

public:

    void twinkleRandom();

    void theaterChaseRainbow();

    void mixedRainbow();

    void bpm();

    void colorWipe(byte red, byte green, byte blue);

    static RgbColor Wheel(uint8_t WheelPos);

    void solidRainbow();

    void randomColors();

    void rainbowColors();

    void meteor();

    void colorWaterfall();

    void randomMarquee();

    void rainbowMarquee();

    void pulsing_rainbow();

    void christmas();

    void fire(int cooling, int sparking, int speedDelay);
    void freeFireBuffer();

    void rainbow(boolean slowdown);
};

#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
