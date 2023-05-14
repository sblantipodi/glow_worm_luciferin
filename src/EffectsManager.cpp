/*
  EffectsManager.cpp - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2023  Davide Perini

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

RgbColor color;
uint8_t bmpPos = 0;

/**
 * Fire effect
 * @param cooling config effect param
 * @param sparking config effect param
 * @param speedDelay config effect param
 * @param dynamicLedNum config effect param
 */
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
    byte t192 = (byte) round((heat[j] / 255.0) * 191);
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

/**
 * Twinkle effect
 * @param count config effect param
 * @param speedDelay config effect param
 * @param onlyOne config effect param
 * @param dynamicLedNum config effect param
 */
void EffectsManager::twinkleRandom(int count, int speedDelay, boolean onlyOne, int dynamicLedNum) {

  LedManager::setColor(1, 1, 1);
  for (int i = 0; i < count; i++) {
    ledManager.setPixelColor(random(dynamicLedNum), random(0, 255), random(0, 255), random(0, 255));
    ledManager.ledShow();
    delay(speedDelay);
    if (onlyOne) {
      LedManager::setColor(1, 1, 1);
    }
  }
  delay(speedDelay);

}

byte * WheelByte(byte WheelPos) {
  static byte c[3];

  if(WheelPos < 85) {
    c[0]=WheelPos * 3;
    c[1]=255 - WheelPos * 3;
    c[2]=0;
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    c[0]=255 - WheelPos * 3;
    c[1]=0;
    c[2]=WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0]=0;
    c[1]=WheelPos * 3;
    c[2]=255 - WheelPos * 3;
  }

  return c;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(uint8_t WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

uint16_t iWipe;
void EffectsManager::colorWipe(int dynamicLedNum, byte rw, byte gw, byte bw) {
  if (iWipe < dynamicLedNum) {
    if (millis() - lastAnim >= 15) {
      lastAnim = millis();
      ledManager.setPixelColor(iWipe, rw, gw, bw);
      ledManager.ledShow();
      iWipe++;

    }
  } else {
    iWipe = 0;
    if (bmpPos >= 5) {
      bmpPos = -1;
    }
    bmpPos++;
  }
}

/**
 * Theater chase rainbow effect
 * @param dynamicLedNum number of leds
 */
void EffectsManager::theaterChaseRainbow(int dynamicLedNum) {

  switch (bmpPos) {
    case 0: colorWipe(dynamicLedNum, 0, 255, 255); break;
    case 1: colorWipe(dynamicLedNum, 0, 255, 0); break;
    case 2: colorWipe(dynamicLedNum, 255, 255, 0); break;
    case 3: colorWipe(dynamicLedNum, 255, 0, 0); break;
    case 4: colorWipe(dynamicLedNum, 255, 0, 255); break;
    case 5: colorWipe(dynamicLedNum, 0, 0, 255); break;
  }

}

/**
 * Mixed Rainbow effect
 * @param dynamicLedNum number of leds
 */
uint16_t jMixed = 0;
uint16_t Mixed = 0;
void EffectsManager::mixedRainbow(int dynamicLedNum) {

  byte *c;
  if (Mixed == 3) {
    jMixed++;
  }
  if (jMixed < 256) {     // cycle all 256 colors in the wheel
    if (Mixed < 3) {
      if (millis() - lastAnim >= 50) {
        lastAnim = millis();
        for (int i = 0; i < dynamicLedNum; i = i + 3) {
          c = WheelByte((i + jMixed) % 255);
          ledManager.setPixelColor(i + Mixed, *c, *(c + 1), *(c + 2));    //turn every third pixel on
        }
        ledManager.ledShow();
        for (int i = 0; i < dynamicLedNum; i = i + 3) {
          ledManager.setPixelColor(i + Mixed, 0, 0, 0);        //turn every third pixel off
        }
        Mixed++;
      }
    } else {
      Mixed = 0;
    }
  } else {
    jMixed = 0;
  }

}

void setAll(int dynamicLedNum, byte red, byte green, byte blue) {
  for(int i = 0; i < dynamicLedNum; i++ ) {
    ledManager.setPixelColor(i, red, green, blue);
  }
  ledManager.ledShow();
}

void FadeInOut(int dynamicLedNum, byte red, byte green, byte blue){
  float r, g, b;

  for(int k = 0; k < 256; k=k+1) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(dynamicLedNum, r,g,b);
    ledManager.ledShow();
  }

  for(int k = 255; k >= 0; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(dynamicLedNum, r,g,b);
    ledManager.ledShow();
  }
  bmpPos += 10;
  if (bmpPos > 255) {
    bmpPos = 0;
  }

}

/**
 * BPM effect
 */
void EffectsManager::bpm(int dynamicLedNum) {

  color = Wheel(bmpPos);
  FadeInOut(dynamicLedNum, color.R, color.G, color.B);

}

/**
 * Rainbow effect
 * @param dynamicLedNum number of leds
 */
byte *cT;
uint16_t iT, jT;
void EffectsManager::rainbow(int dynamicLedNum) {

  if (jT<256*5) { // 5 cycles of all colors on wheel
    if (millis() - lastAnim >= 2) {
      lastAnim = millis();
      for (iT = 0; iT < dynamicLedNum; iT++) {
        cT = WheelByte(((iT * 256 / dynamicLedNum) + jT) & 255);
        ledManager.setPixelColor(iT, *cT, *(cT + 1), *(cT + 2));
      }
      ledManager.ledShow();
      jT++;
    }
  } else {
    jT = 0;
  }

}

/**
 * Solid rainbow effect
 * @param dynamicLedNum number of leds
 */
int xSolidRainbow = 0;
int ySolidRainbow = 0;
void EffectsManager::solidRainbow(int dynamicLedNum) {

  if (xSolidRainbow <= 9) { //9 cycles of rainbow color
    if (ySolidRainbow < 360) {//360 shades - NeoPixelBus uses float
      if (millis() - lastAnim >= 100) {
        lastAnim = millis();
        RgbColor rgb = RgbColor(HslColor(ySolidRainbow / 360.0f, 1.0f, 0.25f));
        for (int i = 0; i < dynamicLedNum; i++) {
          ledManager.setPixelColor(i, rgb.R, rgb.G, rgb.B);
        }
        ledManager.ledShow();
        ySolidRainbow++;
        xSolidRainbow++;
      }
    } else {
      ySolidRainbow = 0;
    }
  } else {
    xSolidRainbow = 0;
  }

}
