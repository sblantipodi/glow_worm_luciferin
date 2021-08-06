/*
  EffectsManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "EffectsManager.h"

void EffectsManager::fire(void (*ledShowCallback)(), void (*setPixelColorCallback)(int, uint8_t r, uint8_t g, uint8_t b),
                          int cooling, int sparking, int speedDelay, int dynamicLedNum) {

  static byte heat[511];
  int cooldown;
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < dynamicLedNum; i++) {
    cooldown = random(0, ((cooling * 10) / dynamicLedNum) + 2);
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= dynamicLedNum - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }
  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < dynamicLedNum; j++) {
    // Scale 'heat' down from 0-255 to 0-191
    byte t192 = round((heat[j]/255.0)*191);
    // calculate ramp up from
    byte heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252
    // figure out which third of the spectrum we're in:
    if( t192 > 0x80) {                     // hottest
      setPixelColorCallback(j, 255, 255, heatramp);
    } else if( t192 > 0x40 ) {             // middle
      setPixelColorCallback(j, 255, heatramp, 0);
    } else {                               // coolest
      setPixelColorCallback(j, heatramp, 0, 0);
    }
  }
  ledShowCallback();
  delay(speedDelay);

}

