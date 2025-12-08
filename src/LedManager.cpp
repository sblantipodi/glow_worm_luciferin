/*
  LedManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "LedManager.h"

/**
 * Show LEDs
 */
void LedManager::ledShow() const {
#if defined(ARDUINO_ARCH_ESP32)
  switch (colorMode) {
    case 1:
      ledsEsp32->Show();
      break;
    case 2:
    case 3:
    case 4:
      ledsEsp32Rgbw->Show();
      break;
    case 5:
      ledsEsp32DotStar->Show();
      break;
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
      case 5:
        ledsDotStar->Show();
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
      case 5:
        ledsDotStar->Show();
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
      case 5:
        ledsDotStar->Show();
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
byte *colorKtoRGB(byte *rgb) {
  float r, g, b;
  float temp = (float) whiteTempInUse - 10;
  if (temp <= 66) {
    r = 255;
    g = (float) round(99.4708025861 * log(temp) - 161.1195681661);
    if (temp <= 19) {
      b = 0;
    } else {
      b = (float) round(138.5177312231 * log((temp - 10)) - 305.0447927307);
    }
  } else {
    r = (float) round(329.698727446 * pow((temp - 60), -0.1332047592));
    g = (float) round(288.1221695283 * pow((temp - 60), -0.0755148492));
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
  return (c && brightness) > 0 ? (c * ((brightness * 100) / 255)) / 100 : c;
}

/**
 * Set color order for RGB
 * @param r color channel
 * @param g color channel
 * @param b color channel
 * @return rgb
 */
RgbColor setColorOrder(uint8_t r, uint8_t g, uint8_t b) {
  switch (colorOrder) {
    case 1: return RgbColor(g, r, b);
    case 2: return RgbColor(r, g, b);
    case 3: return RgbColor(b, g, r);
    case 4: return RgbColor(b, r, g);
    case 5: return RgbColor(r, b, g);
    case 6: return RgbColor(g, b, r);
    default: return RgbColor(r, g, b);
  }
}

/**
 * Set color order for RGBW
 * @param r color channel
 * @param g color channel
 * @param b color channel
 * @param w color channel
 * @return rgb
 */
RgbwColor setColorOrderWhite(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  switch (colorOrder) {
    case 1: return RgbwColor(g, r, b, w);
    case 2: return RgbwColor(r, g, b, w);
    case 3: return RgbwColor(b, g, r, w);
    case 4: return RgbwColor(b, r, g, w);
    case 5: return RgbwColor(r, b, g, w);
    case 6: return RgbwColor(g, b, r, w);
    // Swap white channel g, r, b, w
    case 7: return RgbwColor(g, r, w, b);
    case 8: return RgbwColor(g, w, b, r);
    case 9: return RgbwColor(w, r, b, g);
    // Swap white channel r, g, b, w
    case 10: return RgbwColor(r, g, w, b);
    case 11: return RgbwColor(r, w, b, g);
    case 12: return RgbwColor(w, g, b, r);
    // Swap white channel b, g, r, w
    case 13: return RgbwColor(b, g, w, r);
    case 14: return RgbwColor(b, w, r, g);
    case 15: return RgbwColor(w, g, r, b);
    // Swap white channel b, r, g, w
    case 16: return RgbwColor(b, r, w, g);
    case 17: return RgbwColor(b, w, g, r);
    case 18: return RgbwColor(w, r, g, b);
    // Swap white channel r, b, g, w
    case 19: return RgbwColor(r, b, w, g);
    case 20: return RgbwColor(r, w, g, b);
    case 21: return RgbwColor(w, b, g, r);
    // Swap white channel g, b, r, w
    case 22: return RgbwColor(g, b, w, r);
    case 23: return RgbwColor(g, w, r, b);
    case 24: return RgbwColor(w, b, r, g);
    default: return RgbwColor(r, g, b, w);
  }
}

uint16_t lastKelvin = 0;
byte colorCorrectionRGB[] = {0, 0, 0};

/**
 * Apply white temp correction on RGB color.
 * This function kicks in is Luciferin is not in bias light mode.
 *
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @return RGB color
 */
RgbColor calculateRgbMode(uint8_t r, uint8_t g, uint8_t b) {
  if (whiteTempInUse != WHITE_TEMP_CORRECTION_DISABLE
    && (framerate == 0 && framerateSerial == 0)) {
    if (lastKelvin != whiteTempInUse) {
      colorKtoRGB(colorCorrectionRGB);
    }
    lastKelvin = whiteTempInUse;
    byte rgb[3];
    rgb[0] = ((uint16_t) colorCorrectionRGB[0] * r) / 255; // correct R
    rgb[1] = ((uint16_t) colorCorrectionRGB[1] * g) / 255; // correct G
    rgb[2] = ((uint16_t) colorCorrectionRGB[2] * b) / 255; // correct B
    return setColorOrder(applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2]));
  } else {
    return setColorOrder(applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b));
  }
}

/**
 * Apply white temp correction on RGB color and calculate W channel
 * colorMode can be: 1 = RGB, 2 = RGBW Accurate, 3 = RGBW Brighter, 4 = RGBW RGB only
 * This function kicks in is Luciferin is not in bias light mode.
 *
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
    r -= w;
    g -= w;
    b -= w;
  } else if (colorMode == 4) {
    // RGB only, turn off white led
    w = 0;
  }
  if (whiteTempInUse != WHITE_TEMP_CORRECTION_DISABLE
    && (framerate == 0 && framerateSerial == 0)) {
    if (lastKelvin != whiteTempInUse) {
      colorKtoRGB(colorCorrectionRGB);
    }
    lastKelvin = whiteTempInUse;
    byte rgb[3];
    rgb[0] = ((uint16_t) colorCorrectionRGB[0] * r) / 255; // correct R
    rgb[1] = ((uint16_t) colorCorrectionRGB[1] * g) / 255; // correct G
    rgb[2] = ((uint16_t) colorCorrectionRGB[2] * b) / 255; // correct B
    return setColorOrderWhite(applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2]), applyBrightnessCorrection(w));
  } else {
    return setColorOrderWhite(applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b), applyBrightnessCorrection(w));
  }
}

/**
 * Set pixel color
 * @param index LED num
 * @param r red channel
 * @param g green channel
 * @param b blu channel
 */
void LedManager::setPixelColor(uint16_t index, uint8_t rToOrder, uint8_t gToOrder, uint8_t bToOrder) const {
  RgbColor rgbColor;
  RgbwColor rgbwColor;
  switch (colorMode) {
    case 1:
      rgbColor = calculateRgbMode(rToOrder, gToOrder, bToOrder);
      break;
    case 2:
    case 3:
    case 4:
      rgbwColor = calculateRgbwMode(rToOrder, gToOrder, bToOrder);
      break;
    case 5:
      rgbColor = calculateRgbMode(rToOrder, gToOrder, bToOrder);
      break;
  }
#if defined(ARDUINO_ARCH_ESP32)
  switch (colorMode) {
    case 1:
      ledsEsp32->SetPixelColor(index, rgbColor);
      break;
    case 2:
    case 3:
    case 4:
      ledsEsp32Rgbw->SetPixelColor(index, rgbwColor);
      break;
    case 5:
      ledsEsp32DotStar->SetPixelColor(index, rgbColor);
      break;
  }
#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 1:
        ledsDma->SetPixelColor(index, rgbColor); break;
        break;
      case 2:
      case 3:
      case 4:
        ledsDmaRgbw->SetPixelColor(index, rgbwColor); break;
        break;
      case 5:
        ledsDotStar->SetPixelColor(index, rgbColor); break;
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 1:
        ledsUart->SetPixelColor(index, rgbColor); break;
        break;
      case 2:
      case 3:
      case 4:
        ledsUartRgbw->SetPixelColor(index, rgbwColor); break;
        break;
      case 5:
        ledsDotStar->SetPixelColor(index, rgbColor); break;
    }
  } else {
    switch (colorMode) {
      case 1:
        ledsStandard->SetPixelColor(index, rgbColor); break;
        break;
      case 2:
      case 3:
      case 4:
        ledsStandardRgbw->SetPixelColor(index, rgbwColor); break;
        break;
      case 5:
        ledsDotStar->SetPixelColor(index, rgbColor); break;
    }
  }
#endif
}

RgbColor convertRgbwToRgb(RgbwColor colorRGBW) {
  uint8_t red = max(0, colorRGBW.R - colorRGBW.W);
  uint8_t green = max(0, colorRGBW.G - colorRGBW.W);
  uint8_t blue = max(0, colorRGBW.B - colorRGBW.W);
  return RgbColor(red, green, blue);
}

RgbColor LedManager::getPixelColor(uint16_t index) const {
  RgbColor rgbColor;
  RgbwColor rgbwColor;
#if defined(ARDUINO_ARCH_ESP32)
  switch (colorMode) {
    case 1:
      return ledsEsp32->GetPixelColor(index);
    case 2:
    case 3:
    case 4:
      return convertRgbwToRgb(ledsEsp32Rgbw->GetPixelColor(index));
    case 5:
      return ledsEsp32DotStar->GetPixelColor(index);
  }
#else
  if (gpioInUse == 3) {
    switch (colorMode) {
      case 1:
        return ledsDma->GetPixelColor(index);
      case 2:
      case 3:
      case 4:
        return convertRgbwToRgb(ledsDmaRgbw->GetPixelColor(index));
      case 5:
        return ledsDotStar->GetPixelColor(index);
    }
  } else if (gpioInUse == 2) {
    switch (colorMode) {
      case 1:
        return ledsUart->GetPixelColor(index);
      case 2:
      case 3:
      case 4:
        return convertRgbwToRgb(ledsUartRgbw->GetPixelColor(index));
      case 5:
        return ledsDotStar->GetPixelColor(index);
    }
  } else {
    switch (colorMode) {
      case 1:
        return ledsStandard->GetPixelColor(index);
      case 2:
      case 3:
      case 4:
        return convertRgbwToRgb(ledsStandardRgbw->GetPixelColor(index));
      case 5:
        return ledsDotStar->GetPixelColor(index);
    }
  }
#endif
  return RgbColor(0,0,0);
}

/**
 * Clean the LEDs before reinit
 */
void LedManager::cleanLEDs() {
  boolean cleared = false;
#if defined(ARDUINO_ARCH_ESP32)
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
  } else if (ledsEsp32DotStar != NULL) {
    while (!ledsEsp32DotStar->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32DotStar;
    ledsEsp32DotStar = NULL;
  }
#endif
#if defined(ESP8266)
  if (ledsDma != nullptr) {
    while (!ledsDma->CanShow()) { yield(); }
    cleared = true;
    delete ledsDma;
    ledsDma = nullptr;
  }
  if (ledsDmaRgbw != nullptr) {
    while (!ledsDmaRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsDmaRgbw;
    ledsDmaRgbw = nullptr;
  }
  if (ledsUart != nullptr) {
    while (!ledsUart->CanShow()) { yield(); }
    cleared = true;
    delete ledsUart;
    ledsUart = nullptr;
  }
  if (ledsUartRgbw != nullptr) {
    while (!ledsUartRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsUartRgbw;
    ledsUartRgbw = nullptr;
  }
  if (ledsStandard != nullptr) {
    while (!ledsStandard->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandard;
    ledsStandard = nullptr;
  }
  if (ledsStandardRgbw != nullptr) {
    while (!ledsStandardRgbw->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandardRgbw;
    ledsStandardRgbw = nullptr;
  }
  if (ledsDotStar != nullptr) {
    while (!ledsDotStar->CanShow()) { yield(); }
    cleared = true;
    delete ledsDotStar;
    ledsDotStar = nullptr;
  }
#endif
  if (cleared) {
    Serial.println("LEDs cleared");
  }}

/**
 * Init led strip. No hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initStandard() {
#if defined(ESP8266)
  cleanLEDs();
  ledsStandard = new NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsStandard == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsStandard->Begin();
  ledsStandard->Show();
#endif
}

/**
 * Init led strip RGBW. No hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initStandardRgbw() {
#if defined(ESP8266)
  cleanLEDs();
  ledsStandardRgbw = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266BitBangSk6812Method >(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsStandardRgbw == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));

  flushSerial();
  ledsStandardRgbw->Begin();
  ledsStandardRgbw->Show();
#endif
}

/**
 * Init led strip RGB. Hardware UART, GPIO2, yes serial read/writeHardware UART, GPIO2, yes serial read/write
 */
void LedManager::initUart() {
#if defined(ESP8266)
  cleanLEDs();
  ledsUart = new NeoPixelBus<NeoRgbFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
  if (ledsUart == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsUart->Begin();
  ledsUart->Show();
#endif
}

/**
 * Init led strip RGBW. Hardware UART, GPIO2, yes serial read/write
 */
void LedManager::initUartRgbw() {
#if defined(ESP8266)
  cleanLEDs();
  ledsUartRgbw = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Sk6812Method>(dynamicLedNum, 2); // and recreate with new count
  if (ledsUartRgbw == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsUartRgbw->Begin();
  ledsUartRgbw->Show();
#endif
}

/**
 * Init led strip RGB. Hardware DMA, GPIO3, no serial read, yes serial write
 */
void LedManager::initDma() {
#if defined(ESP8266)
  cleanLEDs();
  ledsDma = new NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
  if (ledsDma == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsDma->Begin();
  ledsDma->Show();
#endif
}

/**
 * Init led strip RGB. DotStar
 */
void LedManager::initDotStar() {
#if defined(ESP8266)
  cleanLEDs();
  ledsDotStar = new NeoPixelBus<DotStarRgbFeature, DotStarMethod>(dynamicLedNum, gpioClockInUse, gpioInUse); // and recreate with new count
  if (ledsDotStar == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsDotStar->Begin();
  ledsDotStar->Show();
#endif
}

/**
 * Init led strip RGBW. Hardware DMA, GPIO3, no serial read, yes serial write
 */
void LedManager::initDmaRgbw() {
#if defined(ESP8266)
  cleanLEDs();
  ledsDmaRgbw = new NeoPixelBus<NeoRgbwFeature, NeoSk6812Method>(dynamicLedNum, 3); // and recreate with new count
  if (ledsDmaRgbw == nullptr) {
    Serial.println(F("OUT OF MEMORY"));
  }
  Serial.println();
  Serial.println(F("Initializing..."));
  flushSerial();
  ledsDmaRgbw->Begin();
  ledsDmaRgbw->Show();
#endif
}

/**
 * Init led strip RGB. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32() {
#if defined(ARDUINO_ARCH_ESP32)
  cleanLEDs();
  ledsEsp32 = new NeoPixelBus<NeoRgbFeature, NeoWs2812xMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32 == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  } else {
    Serial.println();
    Serial.println(F("Initializing..."));
    flushSerial();
    ledsEsp32->Begin();
    ledsEsp32->Show();
  }
#endif
}

/**
 * Init led strip RGB. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32DotStar() {
#if defined(ARDUINO_ARCH_ESP32)
  cleanLEDs();
  ledsEsp32DotStar = new NeoPixelBus<DotStarRgbFeature, DotStarMethod>(dynamicLedNum, gpioClockInUse, gpioInUse); // and recreate with new count
  if (ledsEsp32DotStar == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  } else {
    Serial.println();
    Serial.println(F("Initializing..."));
    flushSerial();
    ledsEsp32DotStar->Begin();
    ledsEsp32DotStar->Show();
  }
#endif
}

/**
 * Init led strip RGBW. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32Rgbw() {
#if defined(ARDUINO_ARCH_ESP32)
  cleanLEDs();
  ledsEsp32Rgbw = new NeoPixelBus<NeoRgbwFeature, NeoSk6812Method>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsEsp32Rgbw == NULL) {
    Serial.println(F("OUT OF MEMORY"));
  } else {
    Serial.println();
    Serial.println(F("Initializing..."));
    flushSerial();
    ledsEsp32Rgbw->Begin();
    ledsEsp32Rgbw->Show();
  }
#endif
}

/**
 * Init LEDs
 */
void LedManager::initLeds() {
#if defined(ARDUINO_ARCH_ESP32)
  switch (colorMode) {
    case 1:
      initEsp32();
      break;
    case 2:
    case 3:
    case 4:
      initEsp32Rgbw();
      break;
    case 5:
      initEsp32DotStar();
      break;
  }
#else
  if (gpioInUse == 3) {
      switch (colorMode) {
        case 1:
          initDma();
          break;
        case 2:
        case 3:
        case 4:
          initDmaRgbw();
          break;
        case 5:
          initDotStar();
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1:
          initUart();
          break;
        case 2:
        case 3:
        case 4:
          initUartRgbw();
          break;
        case 5:
          initDotStar();
      }
    } else {
      switch (colorMode) {
        case 1:
          initStandard();
          break;
        case 2:
        case 3:
        case 4:
          initStandardRgbw();
          break;
        case 5:
          initDotStar();
      }
    }
#endif
}

/**
 * Workaround to flush() that is now blocking on ESP32-S3
 * https://github.com/espressif/arduino-esp32/issues/7554
 */
void LedManager::flushSerial() {
#if CONFIG_IDF_TARGET_ESP32 || defined(ESP8266)
  Serial.flush();
#else
  while (Serial.available()) {
    Serial.read();
  }
#endif
}

/**
 * Set color mode received by the Firefly Luciferin software
 * @param colorMode
 */
void LedManager::setColorMode(int colorModeToUse) {
  Serial.println("CHANGING COLOR MODE");
  JsonDocument colorModeDoc;
  colorModeDoc[COLOR_MODE_PARAM] = colorModeToUse;
  BootstrapManager::writeToLittleFS(colorModeDoc, COLOR_MODE_FILENAME);
  delay(20);
}

/**
 * Set color mode received by the Firefly Luciferin software
 * @param colorMode
 */
void LedManager::setColorOrder(int colorOrderToUse) {
  Serial.println("CHANGING COLOR MODE");
  JsonDocument colorOrderDoc;
  colorOrderDoc[COLOR_ORDER_PARAM] = colorOrderToUse;
  BootstrapManager::writeToLittleFS(colorOrderDoc, COLOR_ORDER_FILENAME);
  delay(20);
}

/**
 * Set LDR params received by the Firefly Luciferin software
 * @param ldrEnabledToSet LDR enabled or disabled
 * @param ldrTurnOffToSet Turn off LEDs before LDR readings
 * @param ldrIntervalToSet Interval between readings
 * @param minLdr min brightness when using LDR
 * @param ledOnParam turn on LEDs automatically at boot
 */
void LedManager::setLdr(boolean ldrEnabledToSet, boolean ldrTurnOffToSet, uint8_t ldrIntervalToSet, uint8_t minLdr) {
  Serial.println(F("CHANGING LDR"));
  previousMillisLDR = 0;
  JsonDocument ldrDoc;
  ldrDoc[LDR_PARAM] = ldrEnabledToSet;
  ldrDoc[LDR_TO_PARAM] = ldrTurnOffToSet;
  ldrDoc[LDR_INTER_PARAM] = ldrIntervalToSet;
  ldrDoc[MIN_LDR_PARAM] = minLdr;
  BootstrapManager::writeToLittleFS(ldrDoc, LDR_FILENAME);
  delay(20);
}


/**
 * Set LDR params received by the Firefly Luciferin software
 * @param maxLdr value used during the calibration
 */
void LedManager::setLdr(int maxLdr) {
  Serial.println(F("CHANGING LDR"));
  JsonDocument ldrDoc;
  ldrDoc[MAX_LDR_PARAM] = maxLdr;
  BootstrapManager::writeToLittleFS(ldrDoc, LDR_CAL_FILENAME);
  delay(20);
}

/**
 * Set LDR params received by the Firefly Luciferin software
 * @param ldrEnabledToSet LDR enabled or disabled
 * @param ldrTurnOffToSet Turn off LEDs before LDR readings
 * @param ldrIntervalToSet Interval between readings
 * @param minLdr min brightness when using LDR
 * @param relInv relay iverted
 */
void LedManager::setPins(uint8_t relayPinParam, uint8_t sbPinParam, uint8_t ldrPinParam, bool relInv) {
  Serial.println(F("CHANGING PINs"));
  JsonDocument ldrDoc;
  ldrDoc[RELAY_PIN_PARAM] = relayPinParam;
  ldrDoc[SB_PIN_PARAM] = sbPinParam;
  ldrDoc[LDR_PIN_PARAM] = ldrPinParam;
  ldrDoc[RELAY_INV] = relInv;
  BootstrapManager::writeToLittleFS(ldrDoc, PIN_FILENAME);
  delay(200);
#if defined(ARDUINO_ARCH_ESP32)
  ESP.restart();
#elif defined(ESP8266)
  EspClass::restart();
#endif
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
 * Set color mode, store it, reinit led strip
 * @param newColorOrder color mode to use
 */
void LedManager::setColorOrderInit(uint8_t newColorOrder) {
  if (colorOrder != newColorOrder) {
    setColorOrder(newColorOrder);
    initLeds();
  }
  colorOrder = newColorOrder;
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
  setColorNoSolid(inR, inG, inB);
}

/**
 * Update transition from one color to another
 */
void LedManager::updateTransition() {
  if (!transitioning || currentStep >= totalSteps) return;
  float ratio = (float) currentStep / (float) (totalSteps - 1);
  currentColor = RgbColor::LinearBlend(startColor, endColor, ratio);
  for (int i = 0; i < ledManager.dynamicLedNum; i++) {
    ledManager.setPixelColor(i, currentColor.R, currentColor.G, currentColor.B);
  }
  ledManager.ledShow();
  currentStep++;
  if (currentStep >= totalSteps) {
    ledManager.startColor = ledManager.endColor;
    temporaryDisableImprove = transitioning = false;
  }
}

/**
 * Set led strip color
 * @param inR red color
 * @param inG green color
 * @param inB blu color
 */
void LedManager::setColorNoSolid(uint8_t inR, uint8_t inG, uint8_t inB) {
  if (effect != Effect::GlowWorm && effect != Effect::GlowWormWifi) {
      if (effect != Effect::solid) {
        for (int i = 0; i < ledManager.dynamicLedNum; i++) {
          ledManager.setPixelColor(i, inR, inG, inB);
        }
        ledManager.ledShow();
      } else {
        if (ledManager.transitioning) {
          ledManager.startColor = ledManager.currentColor;
        }
        ledManager.endColor = RgbColor(inR, inG, inB);
        ledManager.currentStep = 0;
        temporaryDisableImprove = ledManager.transitioning = true;
      }
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
 * Set led strip color
 * @param inR red color
 * @param inG green color
 * @param inB blu color
 */
void LedManager::setColorLoop(uint8_t inR, uint8_t inG, uint8_t inB) {
  for (int j = 0; j < 5; j++) {
    if (inR == 0 && inG == 0 && inB == 0) {
      effect = Effect::solid;
    }
    if (effect != Effect::GlowWorm && effect != Effect::GlowWormWifi) {
      for (int i = 0; i < ledManager.dynamicLedNum; i++) {
        ledManager.setPixelColor(i, inR, inG, inB);
      }
      ledManager.ledShow();
    }
    delay(DELAY_10);
  }
}

/**
 * Set numled received by the Firefly Luciferin software
 * @param numLedFromLuciferin int
 */
void LedManager::setNumLed(int numLedFromLuciferin) {
  ledManager.dynamicLedNum = numLedFromLuciferin;
  JsonDocument numLedDoc;
  numLedDoc[LED_NUM_PARAM] = ledManager.dynamicLedNum;
  BootstrapManager::writeToLittleFS(numLedDoc, LED_NUM_FILENAME);
  delay(20);
}

/**
 * Set white temp received by the Firefly Luciferin software
 * @param wt white temp
 */
void LedManager::setWhiteTemp(int wt) {
  Serial.println(F("CHANGING WHITE TEMP"));
  whiteTempInUse = wt;
  JsonDocument whiteTempDoc;
  whiteTempDoc[WHITE_TEMP_PARAM] = wt;
  BootstrapManager::writeToLittleFS(whiteTempDoc, WHITE_TEMP_FILENAME);
  delay(20);
}

/**
 * Manage built in LED in GRB color order
 * @param r red
 * @param g green
 * @param b blu
 */
void LedManager::manageBuiltInLed(uint8_t r, uint8_t g, uint8_t b) {
#if defined(LED_BUILTIN)
  if (LED_BUILTIN != gpioInUse) {
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    rgbLedWrite(LED_BUILTIN, r, g, b);
#elif defined(ESP8266)
    if (r > 0 || g > 0 || b > 0) {
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
    }
#endif
  }
#endif
}