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
      case 1: ledsEsp32->Show(); break;
      case 2:
      case 3:
      case 4: ledsEsp32Rgbw->Show(); break;
    }
#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 1:
        ledsDma->Show();
        break;
      case 2:
      case 3:
      case 4:
        ledsDmaRgbw->Show();
        break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 1:
        ledsUart->Show();
        break;
      case 2:
      case 3:
      case 4:
        ledsUartRgbw->Show();
        break;
    }
  } else {
    switch (colorMode) {
      case 1:
        ledsStandard->Show();
        break;
      case 2:
      case 3:
      case 4:
        ledsStandardRgbw->Show();
        break;
    }
  }
#endif

}

/**
 * Apply white balance from color temperature in Kelvin
 * (https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html)
 * @param rgb RGB channel
 * @return RGB channel with color correction
 */
byte* colorKtoRGB(byte* rgb) {

  float r, g, b;
  float temp = whiteTempInUse - 10;
  if (temp <= 66) {
    r = 255;
    g = round(99.4708025861 * log(temp) - 161.1195681661);
    if (temp <= 19) {
      b = 0;
    } else {
      b = round(138.5177312231 * log((temp - 10)) - 305.0447927307);
    }
  } else {
    r = round(329.698727446 * pow((temp - 60), -0.1332047592));
    g = round(288.1221695283 * pow((temp - 60), -0.0755148492));
    b = 255;
  }
  rgb[0] = (uint8_t) constrain(r, 0, 255);
  rgb[1] = (uint8_t) constrain(g, 0, 255);
  rgb[2] = (uint8_t) constrain(b, 0, 255);
  return rgb;

}

/**
 * Apply brightness correction on DMA mode
 * @param b red channel
 * @return corrected brightness
 */
uint8_t applyBrightnessCorrection(int c) {

  return (c && brightness) > 0 ? (c*((brightness*100)/255))/100 : c;

}

uint16_t lastKelvin = 0;
byte colorCorrectionRGB[] = {0, 0, 0};

/**
 * Apply white temp correction on RGB color
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @return RGB color
 */
RgbColor calculateRgbMode(uint8_t r, uint8_t g, uint8_t b) {

  if (whiteTempInUse != WHITE_TEMP_CORRECTION_DISABLE) {
    if (lastKelvin != whiteTempInUse) {
      colorKtoRGB(colorCorrectionRGB);
    }
    lastKelvin = whiteTempInUse;
    byte rgb[3];
    rgb[0] = ((uint16_t) colorCorrectionRGB[0] * r) / 255; // correct R
    rgb[1] = ((uint16_t) colorCorrectionRGB[1] * g) / 255; // correct G
    rgb[2] = ((uint16_t) colorCorrectionRGB[2] * b) / 255; // correct B
    return RgbColor(applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2]));
  } else {
    return RgbColor(applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b));
  }

}

/**
 * Apply white temp correction on RGB color and calculate W channel
 * colorMode can be: 1 = RGB, 2 = RGBW Accurate, 3 = RGBW Brighter, 4 = RGBW RGB only
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @return RGBW color
 */
RgbwColor calculateRgbwMode(uint8_t r, uint8_t g, uint8_t b) {

  uint8_t w;
  w = r < g ? (r < b ? r : b) : (g < b ? g : b);
  if (colorMode == 2) {
    // subtract white in accurate mode
    r -= w; g -= w; b -= w;
  } else if (colorMode == 4) {
    // RGB only, turn off white led
    w = 0;
  }
  if (whiteTempInUse != WHITE_TEMP_CORRECTION_DISABLE) {
    if (lastKelvin != whiteTempInUse) {
      colorKtoRGB(colorCorrectionRGB);
    }
    lastKelvin = whiteTempInUse;
    byte rgb[3];
    rgb[0] = ((uint16_t) colorCorrectionRGB[0] * r) /255; // correct R
    rgb[1] = ((uint16_t) colorCorrectionRGB[1] * g) /255; // correct G
    rgb[2] = ((uint16_t) colorCorrectionRGB[2] * b) /255; // correct B
    return RgbwColor(applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2]), applyBrightnessCorrection(w));
  } else {
    return RgbwColor(applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b), applyBrightnessCorrection(w));
  }

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
    case 1: rgbColor = calculateRgbMode(r, g, b); break;
    case 2:
    case 3:
    case 4: rgbwColor = calculateRgbwMode(r, g, b); break;
  }
#if defined(ESP32)
  switch (colorMode) {
    case 1: ledsEsp32->SetPixelColor(index, rgbColor); break;
    case 2:
    case 3:
    case 4: ledsEsp32Rgbw->SetPixelColor(index, rgbwColor); break;
  }
#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 1: ledsDma->SetPixelColor(index, rgbColor); break;
      case 2:
      case 3:
      case 4: ledsDmaRgbw->SetPixelColor(index, rgbwColor); break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 1: ledsUart->SetPixelColor(index, rgbColor); break;
      case 2:
      case 3:
      case 4: ledsUartRgbw->SetPixelColor(index, rgbwColor); break;
    }
  } else {
    switch (colorMode) {
      case 1: ledsStandard->SetPixelColor(index, rgbColor); break;
      case 2:
      case 3:
      case 4: ledsStandardRgbw->SetPixelColor(index, rgbwColor); break;
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
    while (!ledsEsp32->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32;
    ledsEsp32 = NULL;
  } else if (ledsEsp32Rgbw != NULL) {
    while (!ledsEsp32Rgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32Rgbw;
    ledsEsp32Rgbw = NULL;
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

/**
 * Init led strip. No hardware, ALL GPIO, yes serial read/write
 */
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

/**
 * Init led strip RGBW. No hardware, ALL GPIO, yes serial read/write
 */
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

/**
 * Init led strip RGB. Hardware UART, GPIO2, yes serial read/writeHardware UART, GPIO2, yes serial read/write
 */
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

/**
 * Init led strip RGBW. Hardware UART, GPIO2, yes serial read/write
 */
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

/**
 * Init led strip RGB. Hardware DMA, GPIO3, no serial read, yes serial write
 */
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

/**
 * Init led strip RGBW. Hardware DMA, GPIO3, no serial read, yes serial write
 */
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

/**
 * Init led strip RGB. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32() {

#if defined(ESP32)
  Serial.println(F("Using DMA"));
  cleanLEDs();
  ledsEsp32 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32 == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  } else {
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsEsp32->Begin();
    ledsEsp32->Show();
  }
#endif

}

/**
 * Init led strip RGBW. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32Rgbw() {

#if defined(ESP32)
  Serial.println(F("Using DMA"));
  cleanLEDs();
  ledsEsp32Rgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32Rgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  } else {
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsEsp32Rgbw->Begin();
    ledsEsp32Rgbw->Show();
  }
#endif

}

/**
 * Init LEDs
 */
void LedManager::initLeds() {

  switch (colorMode) {
    case 1: initEsp32(); break;
    case 2:
    case 3:
    case 4: initEsp32Rgbw(); break;
  }
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 1: initDma(); break;
      case 2:
      case 3:
      case 4: initDmaRgbw(); break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 1: initUart(); break;
      case 2:
      case 3:
      case 4: initUartRgbw(); break;
    }
  } else {
    switch (colorMode) {
      case 1: initStandard(); break;
      case 2:
      case 3:
      case 4: initStandardRgbw(); break;
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

/**
 * Set LDR params received by the Firefly Luciferin software
 * @param ldrEnabled LDR enabled or disabled
 * @param ldrContinuous LDR continuous readings
 * @param minLdr min brightness when using LDR
 */
void LedManager::setLdr(boolean ldrEnabled, boolean ldrContinuous, String minLdr) {

  Serial.println(F("CHANGING LDR"));
#if defined(ESP8266)
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[LDR_PARAM] = ldrEnabled;
  ldrDoc[LDR_CONT_PARAM] = ldrContinuous;
  ldrDoc[MIN_LDR_PARAM] = minLdr;
  bootstrapManager.writeToLittleFS(ldrDoc, LDR_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[LDR_PARAM] = ldrEnabled;
  ldrDoc[LDR_CONT_PARAM] = ldrContinuous;
  ldrDoc[MIN_LDR_PARAM] = minLdr;
  bootstrapManager.writeToSPIFFS(ldrDoc, LDR_FILENAME);
#endif
  delay(20);

}


/**
 * Set LDR params received by the Firefly Luciferin software
 * @param maxLdr value used during the calibration
 */
void LedManager::setLdr(int maxLdr) {

  Serial.println(F("CHANGING LDR"));
#if defined(ESP8266)
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[MAX_LDR_PARAM] = maxLdr;
  bootstrapManager.writeToLittleFS(ldrDoc, LDR_CAL_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[MAX_LDR_PARAM] = maxLdr;
  bootstrapManager.writeToSPIFFS(ldrDoc, LDR_CAL_FILENAME);
#endif
  delay(20);

}

/**
 * Set color mode, store it, reinit led strip
 * @param newColorMode color mode to use
 */
void LedManager::setColorModeInit(uint8_t newColorMode) {

  if (colorMode != newColorMode) {
    setColorMode(newColorMode);
  }
  // Do not init leds if it is not required (switch from RGB to RGBW or from RGBW to RGB)
  if ((newColorMode > 1 && colorMode == 1) || (newColorMode == 1 && colorMode > 1)) {
    colorMode = newColorMode;
    initLeds();
  }
  colorMode = newColorMode;

}

/**
 * Set led strip color
 * @param inR red color
 * @param inG green color
 * @param inB blu color
 */
void LedManager::setColor(uint8_t inR, uint8_t inG, uint8_t inB) {

  if (inR == 0 && inG == 0 && inB == 0) {
    effect = Effect::solid;
  }
  if (effect != Effect::GlowWorm && effect != Effect::GlowWormWifi) {
    for (int i = 0; i < ledManager.dynamicLedNum; i++) {
      ledManager.setPixelColor(i, inR, inG, inB);
    }
    ledManager.ledShow();
  }
  Serial.print(F("Setting LEDs: "));
  Serial.print(F("r: "));
  Serial.print(inR);
  Serial.print(F(", g: "));
  Serial.print(inG);
  Serial.print(F(", b: "));
  Serial.println(inB);

}

/**
 * Set numled received by the Firefly Luciferin software
 * @param numLedFromLuciferin int
 */
void LedManager::setNumLed(int numLedFromLuciferin) {

  ledManager.dynamicLedNum = numLedFromLuciferin;
#if defined(ESP8266)
  DynamicJsonDocument numLedDoc(1024);
  numLedDoc[LED_NUM_PARAM] = ledManager.dynamicLedNum;
  bootstrapManager.writeToLittleFS(numLedDoc, LED_NUM_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument numLedDoc(1024);
  numLedDoc[LED_NUM_PARAM] = ledManager.dynamicLedNum;
  bootstrapManager.writeToSPIFFS(numLedDoc, LED_NUM_FILENAME);
  SPIFFS.end();
#endif
  delay(20);

}

/**
 * Set white temp received by the Firefly Luciferin software
 * @param wt white temp
 */
void LedManager::setWhiteTemp(int wt) {

  Serial.println(F("CHANGING WHITE TEMP"));
  whiteTempInUse = wt;
  DynamicJsonDocument whiteTempDoc(1024);
  whiteTempDoc[WHITE_TEMP_PARAM] = wt;
#if defined(ESP8266)
  bootstrapManager.writeToLittleFS(whiteTempDoc, WHITE_TEMP_FILENAME);
#endif
#if defined(ESP32)
  bootstrapManager.writeToSPIFFS(whiteTempDoc, WHITE_TEMP_FILENAME);
  SPIFFS.end();
#endif
  delay(20);

}
