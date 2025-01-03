/*
  EffectsManager.cpp - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2025  Davide Perini

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

RgbColor color = EffectsManager::Wheel(random(0, 255));;
unsigned long preMill = 0;
int position = 0;

/**
 * Fire effect
 * @param cooling config effect param
 * @param sparking config effect param
 * @param speedDelay config effect param
 */
void EffectsManager::fire(int cooling, int sparking, int speedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
  // Step 1.  Cool down every cell a little
  for (int i = 0; i < ledManager.dynamicLedNum; i++) {
    cooldown = random(0, ((cooling * 10) / ledManager.dynamicLedNum) + 2);
    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = ledManager.dynamicLedNum - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if (random(255) < sparking) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }
  // Step 4.  Convert heat to LED colors
  for (int j = 0; j < ledManager.dynamicLedNum; j++) {
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

void EffectsManager::randomColors() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 150) {
    preMill = curMill;
    for (int i = 0; i < ledManager.dynamicLedNum; i++) {
      ledManager.setPixelColor(i, (brightness * random(0, 255)), (brightness * random(0, 255)),
                               (brightness * random(0, 255)));
    }
    ledManager.ledShow();
  }
}

void EffectsManager::rainbowColors() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 20) {
    preMill = curMill;
    static uint8_t hue = 0;
    for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
      RgbColor c = HslColor(hue / 255.0f, 1.0f, 0.5f);
      ledManager.setPixelColor(i, c.R, c.G, c.B);
    }
    ledManager.ledShow();
    hue++;
  }
}

void EffectsManager::meteor() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 10) {
    previousMillis = curMill;
    ledManager.setPixelColor(position, ledManager.red, ledManager.green, ledManager.blue);
    ledManager.setPixelColor(ledManager.dynamicLedNum - 1, 0,0,0);
    if (position > 0) { ledManager.setPixelColor(position - 1, 0,0,0); }
    ledManager.ledShow();
    position = (position + 1) % ledManager.dynamicLedNum;
  }
}

void EffectsManager::colorWaterfall() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 400) {
    preMill = curMill;
    RgbColor c = RgbColor(random(0, 255), random(0, 255), random(0, 255));
    for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) { ledManager.setPixelColor(i, c.R, c.G, c.B); }
    ledManager.ledShow();
    delay(20);
  }
}

uint8_t hue = 0;
int currentPixel = 0;
void EffectsManager::randomMarquee() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 150) {
    preMill = curMill;
    for (int i = 0; i < ledManager.dynamicLedNum; i++) {
      if (i % 3 == currentPixel % 3) {
        ledManager.setPixelColor(i, random(0, 255), random(0, 255), random(0, 255));
      } else {
        ledManager.setPixelColor(i, 0,0,0);
      }
    }
    ledManager.ledShow();
    currentPixel++;
  }
}

void EffectsManager::rainbowMarquee() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 150) {
    preMill = curMill;
    for (int i = 0; i < ledManager.dynamicLedNum; i++) {
      if (i % 3 == currentPixel % 3) {
        RgbColor c = HslColor(((i * 256 / ledManager.dynamicLedNum) + hue) / 255.0f, 1.0f, 0.5f);
        ledManager.setPixelColor(i, c.R, c.G, c.B);
      } else {
        ledManager.setPixelColor(i, 0,0,0);
      }
    }
    hue++;
    ledManager.ledShow();
    currentPixel++;
  }
}

void EffectsManager::pulsing_rainbow() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 20/10) {
    preMill = curMill;
    for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
      RgbColor c = HslColor(((i * 256 / ledManager.dynamicLedNum) + hue) / 255.0f, 1.0f, 0.5f);
      ledManager.setPixelColor(i, c.R, c.G, c.B);
    }
    ledManager.ledShow();
    hue++;
  }
}

void EffectsManager::christmas() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 300) {
    preMill = curMill;
    for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
      RgbColor c = HslColor(hue / 255.0f, 1.0f, 0.5f);
      ledManager.setPixelColor(i, c.R, c.G, c.B);
      hue++;
    }
    ledManager.ledShow();
  }
}

/**
 * Twinkle effect
 * @param count config effect param
 * @param speedDelay config effect param
 * @param onlyOne config effect param
 * @param ledManager.dynamicLedNum config effect param
 */
void EffectsManager::twinkleRandom() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 200) {
    preMill = curMill;
    int pixelIndex = random(ledManager.dynamicLedNum);
    RgbColor c = RgbColor(random(256), random(256), random(256));
    ledManager.setPixelColor(pixelIndex, c.R, c.G, c.B);
    for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
      RgbColor currentColor = ledManager.getPixelColor(i);
      RgbColor dimColor = RgbColor(currentColor.R / 1.05, currentColor.G / 1.05, currentColor.B / 1.05);
      ledManager.setPixelColor(i, dimColor.R, dimColor.G, dimColor.B);
    }
    ledManager.ledShow();
  }
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
RgbColor EffectsManager::Wheel(uint8_t WheelPos) {
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
void EffectsManager::colorWipe(byte rw, byte gw, byte bw) {
  if (iWipe < ledManager.dynamicLedNum) {
    if (millis() - lastAnim >= 15) {
      lastAnim = millis();
      ledManager.setPixelColor(iWipe, rw, gw, bw);
      ledManager.ledShow();
      iWipe++;
    }
  } else {
    color = Wheel(random(0, 255));
    iWipe = 0;
  }
}

/**
 * Theater chase rainbow effect

 */
void EffectsManager::theaterChaseRainbow() {
  colorWipe(color.R, color.G, color.B);
}

/**
 * Mixed Rainbow effect

 */
uint16_t jMixed = 0;
uint16_t Mixed = 0;
void EffectsManager::mixedRainbow() {
  unsigned long curMill = millis();
  if (curMill - preMill >= 500) {
    preMill = curMill;
    byte *c;
    if (Mixed == 3) {
      jMixed++;
    }
    if (jMixed < 256) {     // cycle all 256 colors in the wheel
      if (Mixed < 3) {
        if (millis() - lastAnim >= 20) {
          lastAnim = millis();
          for (int z = 0; z < ledManager.dynamicLedNum; z = z + 3) {
            c = WheelByte((z + jMixed) % 255);
            ledManager.setPixelColor(z + Mixed, *c, *(c + 1), *(c + 2));    //turn every third pixel on
          }
          ledManager.ledShow();
          for (int k = 0; k < ledManager.dynamicLedNum; k = k + 3) {
            ledManager.setPixelColor(k + Mixed, 0, 0, 0);        //turn every third pixel off
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
}

void setAll(int dynamicLedNum, byte red, byte green, byte blue) {
  for(int i = 0; i < dynamicLedNum; i++ ) {
    ledManager.setPixelColor(i, red, green, blue);
  }
  ledManager.ledShow();
}

int kFade = 0;
bool stepFadeIn = true;
void FadeInOut(byte red, byte green, byte blue) {
  float r, g, b;
  if (stepFadeIn && kFade == 255) {
    stepFadeIn = false;
  }
  if (!stepFadeIn && kFade <= 0) {
    stepFadeIn = true;
    color = EffectsManager::Wheel(random(0, 255));
  }
  if (kFade < 256 && stepFadeIn) {
    r = (kFade / 256.0) * red;
    g = (kFade / 256.0) * green;
    b = (kFade / 256.0) * blue;
    setAll(ledManager.dynamicLedNum, r, g, b);
    kFade = kFade + 1;
  } else if (kFade >= 0 && !stepFadeIn) {
    r = (kFade / 256.0) * red;
    g = (kFade / 256.0) * green;
    b = (kFade / 256.0) * blue;
    setAll(ledManager.dynamicLedNum, r, g, b);
    kFade = kFade - 2;
  }
}

/**
 * BPM effect
 */
void EffectsManager::bpm() {

  FadeInOut(color.R, color.G, color.B);
}

/**
 * Rainbow effect
 */
byte *cT;
uint16_t iT, jT;
void EffectsManager::rainbow(boolean slowdown) {
  unsigned long curMill = millis();
  int raindelay = slowdown ? 100 : 0;
  if (curMill - preMill >= raindelay) {
    preMill = curMill;
    if (jT < 256 * 5) { // 5 cycles of all colors on wheel
      if (millis() - lastAnim >= 2) {
        lastAnim = millis();
        for (iT = 0; iT < ledManager.dynamicLedNum; iT++) {
          cT = WheelByte(((iT * 256 / ledManager.dynamicLedNum) + jT) & 255);
          ledManager.setPixelColor(iT, *cT, *(cT + 1), *(cT + 2));
        }
        ledManager.ledShow();
        jT++;
      }
    } else {
      jT = 0;
    }
  }
}

/**
 * Solid rainbow effect
 */
int xSolidRainbow = 0;
int ySolidRainbow = 0;
void EffectsManager::solidRainbow() {
  if (xSolidRainbow <= 9) { //9 cycles of rainbow color
    if (ySolidRainbow < 360) {//360 shades - NeoPixelBus uses float
      if (millis() - lastAnim >= 100) {
        lastAnim = millis();
        RgbColor rgb = RgbColor(HslColor(ySolidRainbow / 360.0f, 1.0f, 0.25f));
        for (int i = 0; i < ledManager.dynamicLedNum; i++) {
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
