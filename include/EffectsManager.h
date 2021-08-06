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


class EffectsManager {

  public:
    void fire(void (*ledShowCallback)(), void (*setPixelColorCallback)(int, uint8_t r, uint8_t g, uint8_t b),
              int cooling, int sparking, int speedDelay, int dynamicLedNum);

};


#endif //GLOW_WORM_LUCIFERIN_EFFECTSMANAGER_H
