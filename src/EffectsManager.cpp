/*
  EffectsManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "EffectsManager.h"

void EffectsManager::fire(int cooling, int sparking, int speedDelay, int dynamicLedNum) {

  static byte heat[NUM_LEDS];
  int cooldown;
  // Step 1.  Cool down every cell a little
  for (int i = 0; i < dynamicLedNum; i++) {
    cooldown = random(0, ((cooling * 10) / dynamicLedNum) + 2);
    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = dynamicLedNum - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if (random(255) < sparking) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }
  // Step 4.  Convert heat to LED colors
  for (int j = 0; j < dynamicLedNum; j++) {
    // Scale 'heat' down from 0-255 to 0-191
    byte t192 = round((heat[j] / 255.0) * 191);
    // calculate ramp up from
    byte heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252
    // figure out which third of the spectrum we're in:
    if (t192 > 0x80) {                     // hottest
      ledManager.setPixelColor(j, 255, 255, heatramp);
    } else if (t192 > 0x40) {             // middle
      ledManager.setPixelColor(j, 255, heatramp, 0);
    } else {                               // coolest
      ledManager.setPixelColor(j, heatramp, 0, 0);
    }
  }
  ledManager.ledShow();
  delay(speedDelay);

}

void EffectsManager::twinkleRandom(int count, int speedDelay, boolean onlyOne, int dynamicLedNum) {

  ledManager.setColor(1, 1, 1);
  for (int i = 0; i < count; i++) {
    ledManager.setPixelColor(random(dynamicLedNum), random(0, 255), random(0, 255), random(0, 255));
    ledManager.ledShow();
    delay(speedDelay);
    if (onlyOne) {
      ledManager.setColor(1, 1, 1);
    }
  }
  delay(speedDelay);

}

void EffectsManager::theaterChaseRainbow(int dynamicLedNum) {

  // cycle all 256 colors in the wheel
  for (int j = 0; j < 256; j++) {
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < dynamicLedNum; i = i + 3) {
        static byte c[3];
        byte wheelPos = (i + j) % 255;
        if (wheelPos < 85) {
          c[0] = wheelPos * 3;
          c[1] = 255 - wheelPos * 3;
          c[2] = 0;
        } else if (wheelPos < 170) {
          wheelPos -= 85;
          c[0] = 255 - wheelPos * 3;
          c[1] = 0;
          c[2] = wheelPos * 3;
        } else {
          wheelPos -= 170;
          c[0] = 0;
          c[1] = wheelPos * 3;
          c[2] = 255 - wheelPos * 3;
        }
        //turn every third pixel on
        ledManager.setPixelColor(i + q, *c, *(c + 1), *(c + 2));
      }
      ledManager.ledShow();
      delay(1);
      for (int i = 0; i < dynamicLedNum; i = i + 3) {
        //turn every third pixel off
        ledManager.setPixelColor(i + q, 0, 0, 0);
      }
    }
  }
}

void EffectsManager::mixedRainbow(void (*checkConnectionCallback)(),
                                  CRGB leds[NUM_LEDS], int dynamicLedNum) {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (millis() - lastAnim >= 10) {
    lastAnim = millis();
    mixedRainboxIndex++;
  }
#elif TARGET_GLOWWORMLUCIFERINLIGHT
  mixedRainboxIndex++;
#endif
  if (mixedRainboxIndex < 256) {
    for (int i = 0; i < dynamicLedNum; i++) {
      leds[i] = scroll((i * 256 / dynamicLedNum + mixedRainboxIndex) % 256);
      ledManager.setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
#ifdef TARGET_GLOWWORMLUCIFERINFULL
      checkConnectionCallback();
#endif
    }
    ledManager.ledShow();
  } else {
    mixedRainboxIndex = 0;
  }

}

// WS2812B LED Strip switches Red and Green
CRGB EffectsManager::scroll(int pos) {
  CRGB color(0, 0, 0);
  if (pos < 85) {
    color.g = 0;
    color.r = ((float) pos / 85.0f) * 255.0f;
    color.b = 255 - color.r;
  } else if (pos < 170) {
    color.g = ((float) (pos - 85) / 85.0f) * 255.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if (pos < 256) {
    color.b = ((float) (pos - 170) / 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
}

void EffectsManager::bpm(CRGB leds[NUM_LEDS], CRGBPalette16 currentPalette, CRGBPalette16 targetPalette) {

  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    ledManager.setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
  }
  ledManager.ledShow();

  EVERY_N_MILLISECONDS(10) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
    {
      gHue++;
    }
  }
  EVERY_N_SECONDS(5) {
    targetPalette = CRGBPalette16(CHSV(random16(), 255, random16(128, 255)),
                                  CHSV(random16(), 255, random16(128, 255)),
                                  CHSV(random16(), 192, random16(128, 255)),
                                  CHSV(random16(), 255, random16(128, 255)));
  }

}

void EffectsManager::rainbow(CRGB leds[NUM_LEDS], int dynamicLedNum) {

  // FastLED's built-in rainbow generator
  thishue++;
  fill_rainbow(leds, dynamicLedNum, thishue, deltahue);
  for (int i = 0; i < dynamicLedNum; i++) {
    ledManager.setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
  }
  ledManager.ledShow();

}

void EffectsManager::solidRainbow(CRGB leds[NUM_LEDS], int dynamicLedNum) {

  // FastLED's built-in rainbow generator
  fill_solid(leds, dynamicLedNum, CHSV(thishue, 255, 255));
  for (int i = 0; i < dynamicLedNum; i++) {
    ledManager.setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
  }
  if (millis()-lastAnimSolidRainbow >= 90) {
    lastAnimSolidRainbow = millis();
    thishue++;
  }
  ledManager.ledShow();

}
