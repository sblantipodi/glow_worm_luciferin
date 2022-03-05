/*
  LedManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "LedManager.h"

/**
 * Show LEDs
 */
void LedManager::ledShow() {

#if defined(ESP32)
  switch (colorMode) {
      case 0: ledsEsp32->Show(); break;
      case 1:
      case 2:
      case 3: ledsEsp32Rgbw->Show(); break;
    }
#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 0:
        ledsDma->Show();
        break;
      case 1:
      case 2:
      case 3:
        ledsDmaRgbw->Show();
        break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 0:
        ledsUart->Show();
        break;
      case 1:
      case 2:
      case 3:
        ledsUartRgbw->Show();
        break;
    }
  } else {
    switch (colorMode) {
      case 0:
        ledsStandard->Show();
        break;
      case 1:
      case 2:
      case 3:
        ledsStandardRgbw->Show();
        break;
    }
  }
#endif

}

/**
 * Apply brightness correction on DMA mode
 * @param b red channel
 * @return corrected brightness
 */
uint8_t applyBrightnessCorrection(int c) {
  return (c && brightness) > 0 ? (c*((brightness*100)/255))/100 : c;
}

/**
 * Apply white temp correcton on DMA mode
 * @param r red channel
 * @return corrected temperature
 */
uint8_t applyWhiteTempRed(uint8_t r) {
  return r > 0 ? applyBrightnessCorrection((whiteTempCorrection[0] * r) / 255) : r;
}
/**
 * Apply white temp correction on DMA mode
 * @param g red channel
 * @return corrected temperature
 */
uint8_t applyWhiteTempGreen(uint8_t g) {
  return g > 0 ? applyBrightnessCorrection((whiteTempCorrection[1] * g) / 255) : g;
}
/**
 * Apply white temp correction on DMA mode
 * @param b red channel
 * @return corrected temperature
 */
uint8_t applyWhiteTempBlue(uint8_t b) {
  return b > 0 ? applyBrightnessCorrection((whiteTempCorrection[2] * b) / 255) : b;
}



RgbColor calculateRgbMode(uint8_t r, uint8_t g, uint8_t b) {

  return RgbColor(applyWhiteTempRed(r), applyWhiteTempGreen(g), applyWhiteTempBlue(b));

}

RgbwColor calculateRgbwMode(uint8_t r, uint8_t g, uint8_t b) {

  uint8_t w;
  w = r < g ? (r < b ? r : b) : (g < b ? g : b);
  // subtract w in accurate mode
  if (colorMode == 1) {
    r -= w; g -= w; b -= w;
  } else if (colorMode == 3) {
    w = 0;
  }
  return RgbwColor(applyWhiteTempRed(r), applyWhiteTempGreen(g), applyWhiteTempBlue(b), w);

}

/**
 * Set pixel color
 * @param index LED num
 * @param r red channel
 * @param g green channel
 * @param b blu channel
 */
void LedManager::setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {

  RgbColor rgbColor;
  RgbwColor rgbwColor;
  switch (colorMode) {
    case 0: rgbColor = calculateRgbMode(r, g, b); break;
    case 1:
    case 2:
    case 3: rgbwColor = calculateRgbwMode(r, g, b); break;
  }

#if defined(ESP32)
  switch (colorMode) {
    case 0: ledsEsp32->SetPixelColor(index, rgbColor); break;
    case 1:
    case 2:
    case 3: ledsEsp32Rgbw->SetPixelColor(index, rgbwColor); break;
  }

#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 0: ledsDma->SetPixelColor(index, rgbColor); break;
      case 1:
      case 2:
      case 3: ledsDmaRgbw->SetPixelColor(index, rgbwColor); break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 0: ledsUart->SetPixelColor(index, rgbColor); break;
      case 1:
      case 2:
      case 3: ledsUartRgbw->SetPixelColor(index, rgbwColor); break;
    }
  } else {
    switch (colorMode) {
      case 0: ledsStandard->SetPixelColor(index, rgbColor); break;
      case 1:
      case 2:
      case 3: ledsStandardRgbw->SetPixelColor(index, rgbwColor); break;
    }
  }
#endif

}

/**
 * Clean the LEDs before reinit
 */
void LedManager::cleanLEDs() {

  boolean cleared = false;
#if defined(ESP32)
  if (ledsEsp32 != NULL) {
    cleared = true;
    delete ledsEsp32;
  } else if (ledsEsp32Rgbw != NULL) {
    cleared = true;
    delete ledsEsp32Rgbw;
  }
#endif
#if defined(ESP8266)
  if (ledsDma != NULL) {
    while (!ledsDma->CanShow()) { yield(); }
    cleared = true;
    delete ledsDma;
    ledsDma = NULL;
  }
  if (ledsDmaRgbw != NULL) {
    while (!ledsDmaRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsDmaRgbw;
    ledsDmaRgbw = NULL;
  }
  if (ledsUart != NULL) {
    while (!ledsUart->CanShow()) { yield(); }
    cleared = true;
    delete ledsUart;
    ledsUart = NULL;
  }
  if (ledsUartRgbw != NULL) {
    while (!ledsUartRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsUartRgbw;
    ledsUartRgbw = NULL;
  }
  if (ledsStandard != NULL) {
    while (!ledsStandard->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandard;
    ledsStandard = NULL;
  }
  if (ledsStandardRgbw != NULL) {
    while (!ledsStandardRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandardRgbw;
    ledsStandardRgbw = NULL;
  }
#endif
  if (cleared) {
    Serial.println("LEDs cleared");
  }

}

void LedManager::initStandard() {
#if defined(ESP8266)
  Serial.println(F("Using Standard"));

  cleanLEDs();
  ledsStandard = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
  if (ledsStandard == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsStandard->Begin();
  ledsStandard->Show();
#endif
}

void LedManager::initStandardRgbw() {
#if defined(ESP8266)
  Serial.println(F("Using Standard RGBW"));

  cleanLEDs();
  ledsStandardRgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
  if (ledsStandardRgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsStandardRgbw->Begin();
  ledsStandardRgbw->Show();
#endif
}

void LedManager::initUart() {
#if defined(ESP8266)
  Serial.println(F("Using UART"));

  cleanLEDs();
  ledsUart = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
  if (ledsUart == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsUart->Begin();
  ledsUart->Show();
#endif
}

void LedManager::initUartRgbw() {
#if defined(ESP8266)
  Serial.println(F("Using UART RGBW"));

  cleanLEDs();
  ledsUartRgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
  if (ledsUartRgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsUartRgbw->Begin();
  ledsUartRgbw->Show();
#endif
}

void LedManager::initDma() {
#if defined(ESP8266)
  Serial.println(F("Using DMA"));

  cleanLEDs();
  ledsDma = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
  if (ledsDma == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsDma->Begin();
  ledsDma->Show();
#endif
}

void LedManager::initDmaRgbw() {
#if defined(ESP8266)
  Serial.println(F("Using DMA RGBW"));

  cleanLEDs();
  ledsDmaRgbw = new NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
  if (ledsDmaRgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));

  Serial.flush();
  ledsDmaRgbw->Begin();
  ledsDmaRgbw->Show();
#endif
}

void initEsp32() {
#if defined(ESP32)
  Serial.println(F("Using DMA"));
  cleanLEDs();
  ledsEsp32 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32 == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));
  Serial.flush();
  ledsEsp32->Begin();
  ledsEsp32->Show();
#endif
}

void initEsp32Rgbw() {
#if defined(ESP32)
  Serial.println(F("Using DMA"));
  cleanLEDs();
  ledsEsp32Rgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32Rgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));
  Serial.flush();
  ledsEsp32Rgbw->Begin();
  ledsEsp32Rgbw->Show();
#endif
}

/**
 * Init LEDs
 */
void LedManager::initLeds() {

  switch (colorMode) {
    case 0: initEsp32(); break;
    case 1:
    case 2:
    case 3: initEsp32Rgbw(); break;
  }

  if (gpioInUse == 3) {
    switch (colorMode) {
      case 0: initDma(); break;
      case 1:
      case 2:
      case 3: initDmaRgbw(); break;
    }
  } else if (gpioInUse == 2) {

    switch (colorMode) {
      case 0: initUart(); break;
      case 1:
      case 2:
      case 3: initUartRgbw(); break;
    }
  } else {

    switch (colorMode) {
      case 0: initStandard(); break;
      case 1:
      case 2:
      case 3: initStandardRgbw(); break;
    }
  }

}



/**
 * Set color mode received by the Firefly Luciferin software
 * @param colorMode
 */
void LedManager::setColorMode(int colorModeToUse) {

  Serial.println("CHANGING COLOR MODE");
#if defined(ESP8266)
  DynamicJsonDocument colorModeDoc(1024);
  colorModeDoc[COLOR_MODE_PARAM] = colorModeToUse;
  bootstrapManager.writeToLittleFS(colorModeDoc, COLOR_MODE_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument colorModeDoc(1024);
  colorModeDoc[COLOR_MODE_PARAM] = colorModeToUse;
  bootstrapManager.writeToSPIFFS(colorModeDoc, COLOR_MODE_FILENAME);
#endif
  delay(20);

}

void LedManager::setColorModeInit(uint8_t newColorMode) {

  if (colorMode != newColorMode) {
    setColorMode(newColorMode);
  }
  if ((newColorMode > 0 && colorMode == 0) || (newColorMode == 0 && colorMode > 0)) {
    colorMode = newColorMode;
    initLeds();
  }
  colorMode = newColorMode;

}

/**
 * Set White Temperature for Color Correction
 * @param whitetemp kelvin
 */
void LedManager::setTemperature(int whitetemp) {

  switch (whitetemp) {
    case 1: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 255; whiteTempCorrection[2] = 255;  break;
    case 2: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 147; whiteTempCorrection[2] = 41;  break;
    case 3: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 197; whiteTempCorrection[2] = 143;  break;
    case 4: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 214; whiteTempCorrection[2] = 170;  break;
    case 5: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 241; whiteTempCorrection[2] = 224;  break;
    case 6: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 250; whiteTempCorrection[2] = 244;  break;
    case 7: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 255; whiteTempCorrection[2] = 251;  break;
    case 8: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 255; whiteTempCorrection[2] = 255;  break;
    case 9: whiteTempCorrection[0] = 201; whiteTempCorrection[1] = 226; whiteTempCorrection[2] = 255;  break;
    case 10: whiteTempCorrection[0] = 64; whiteTempCorrection[1] = 156; whiteTempCorrection[2] = 255;  break;
    case 11: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 224; whiteTempCorrection[2] = 229;  break;
    case 12: whiteTempCorrection[0] = 244; whiteTempCorrection[1] = 255; whiteTempCorrection[2] = 250;  break;
    case 13: whiteTempCorrection[0] = 212; whiteTempCorrection[1] = 235; whiteTempCorrection[2] = 255;  break;
    case 14: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 244; whiteTempCorrection[2] = 242;  break;
    case 15: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 239; whiteTempCorrection[2] = 247;  break;
    case 16: whiteTempCorrection[0] = 167; whiteTempCorrection[1] = 1; whiteTempCorrection[2] = 255;  break;
    case 17: whiteTempCorrection[0] = 216; whiteTempCorrection[1] = 247; whiteTempCorrection[2] = 255;  break;
    case 18: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 209; whiteTempCorrection[2] = 178;  break;
    case 19: whiteTempCorrection[0] = 242; whiteTempCorrection[1] = 252; whiteTempCorrection[2] = 255;  break;
    case 20: whiteTempCorrection[0] = 255; whiteTempCorrection[1] = 183; whiteTempCorrection[2] = 76;  break;
  }

}