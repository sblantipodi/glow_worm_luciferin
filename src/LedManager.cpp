#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"
/*
  LedManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "LedManager.h"

/**
 * Show LEDs
 */
void LedManager::ledShow() const {

#if defined(ESP32)
  if (colorOrder == 1) {
    switch (colorMode) {
      case 1:
        ledsEsp32->Show();
        break;
      case 2:
      case 3:
      case 4:
        ledsEsp32Rgbw->Show();
        break;
    }
  } else if (colorOrder == 2) {
    switch (colorMode) {
      case 1:
        ledsEsp32Inverted->Show();
        break;
      case 2:
      case 3:
      case 4:
        ledsEsp32RgbwInverted->Show();
        break;
    }
  } else if (colorOrder == 3) {
    switch (colorMode) {
      case 1:
      case 2:
      case 3:
      case 4:
        ledsEsp32Bgr->Show();
        break;
    }
  }
#else
  if (colorOrder == 1) {
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
  } else if (colorOrder == 2) {
    if (gpioInUse == 3) {
      switch (colorMode) {
        case 1:
          ledsDmaInverted->Show();
          break;
        case 2:
        case 3:
        case 4:
          ledsDmaRgbwInverted->Show();
          break;
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1:
          ledsUartInverted->Show();
          break;
        case 2:
        case 3:
        case 4:
          ledsUartRgbwInverted->Show();
          break;
      }
    } else {
      switch (colorMode) {
        case 1:
          ledsStandardInverted->Show();
          break;
        case 2:
        case 3:
        case 4:
          ledsStandardRgbwInverted->Show();
          break;
      }
    }
  } else if (colorOrder == 3) {
    if (gpioInUse == 3) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          ledsDmaBgr->Show();
          break;
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          ledsUartBgr->Show();
          break;
      }
    } else {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          ledsStandardBgr->Show();
          break;
      }
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
    return {applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2])};
  } else {
    return {applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b)};
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
    r -= w;
    g -= w;
    b -= w;
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
    rgb[0] = ((uint16_t) colorCorrectionRGB[0] * r) / 255; // correct R
    rgb[1] = ((uint16_t) colorCorrectionRGB[1] * g) / 255; // correct G
    rgb[2] = ((uint16_t) colorCorrectionRGB[2] * b) / 255; // correct B
    return {applyBrightnessCorrection(rgb[0]), applyBrightnessCorrection(rgb[1]), applyBrightnessCorrection(rgb[2]),
            applyBrightnessCorrection(w)};
  } else {
    return {applyBrightnessCorrection(r), applyBrightnessCorrection(g), applyBrightnessCorrection(b),
            applyBrightnessCorrection(w)};
  }

}

/**
 * Set pixel color
 * @param index LED num
 * @param r red channel
 * @param g green channel
 * @param b blu channel
 */
void LedManager::setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) const {

  RgbColor rgbColor;
  RgbwColor rgbwColor;
  if (colorOrder == 1 || colorOrder == 2) {
    switch (colorMode) {
      case 1:
        rgbColor = calculateRgbMode(r, g, b);
        break;
      case 2:
      case 3:
      case 4:
        rgbwColor = calculateRgbwMode(r, g, b);
        break;
    }
  } else if (colorOrder == 3) {
    switch (colorMode) {
      case 1:
      case 2:
      case 3:
      case 4:
        rgbColor = calculateRgbMode(r, g, b);
        break;
    }
  }
#if defined(ESP32)
  if (colorOrder == 1) {
    switch (colorMode) {
      case 1:
        ledsEsp32->SetPixelColor(index, rgbColor);
        break;
      case 2:
      case 3:
      case 4:
        ledsEsp32Rgbw->SetPixelColor(index, rgbwColor);
        break;
    }
  } else if (colorOrder == 2) {
    switch (colorMode) {
      case 1:
        ledsEsp32Inverted->SetPixelColor(index, rgbColor);
        break;
      case 2:
      case 3:
      case 4:
        ledsEsp32RgbwInverted->SetPixelColor(index, rgbwColor);
        break;
    }
  } else if (colorOrder == 3) {
    switch (colorMode) {
      case 1:
      case 2:
      case 3:
      case 4:
        ledsEsp32Bgr->SetPixelColor(index, rgbColor);
        break;
    }
  }
#else
  if (colorOrder == 1) {
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
  } else if (colorOrder == 2) {
    if (gpioInUse == 3) {
      switch (colorMode) {
        case 1: ledsDmaInverted->SetPixelColor(index, rgbColor); break;
        case 2:
        case 3:
        case 4: ledsDmaRgbwInverted->SetPixelColor(index, rgbwColor); break;
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1: ledsUartInverted->SetPixelColor(index, rgbColor); break;
        case 2:
        case 3:
        case 4: ledsUartRgbwInverted->SetPixelColor(index, rgbwColor); break;
      }
    } else {
      switch (colorMode) {
        case 1: ledsStandardInverted->SetPixelColor(index, rgbColor); break;
        case 2:
        case 3:
        case 4: ledsStandardRgbwInverted->SetPixelColor(index, rgbwColor); break;
      }
    }
  } else if (colorOrder == 3) {
    if (gpioInUse == 3) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4: ledsDmaBgr->SetPixelColor(index, rgbColor); break;
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4: ledsUartBgr->SetPixelColor(index, rgbColor); break;
      }
    } else {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4: ledsStandardBgr->SetPixelColor(index, rgbColor); break;
      }
    }
  }
#endif

}

/**
 * Clean the LEDs before reinit
 */
void LedManager::cleanLEDs() {

  cleanGrb();
  cleanRgb();
  cleanBgr();

}

void LedManager::cleanGrb() {

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
#endif
  if (cleared) {
    Serial.println("LEDs cleared");
  }

}

void LedManager::cleanRgb() {

  boolean cleared = false;
#if defined(ESP32)
  if (ledsEsp32Inverted != NULL) {
    while (!ledsEsp32Inverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32Inverted;
    ledsEsp32Inverted = NULL;
  } else if (ledsEsp32RgbwInverted != NULL) {
    while (!ledsEsp32RgbwInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32RgbwInverted;
    ledsEsp32RgbwInverted = NULL;
  }
#endif
#if defined(ESP8266)
  if (ledsDmaInverted != nullptr) {
    while (!ledsDmaInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsDmaInverted;
    ledsDmaInverted = nullptr;
  }
  if (ledsDmaRgbwInverted != nullptr) {
    while (!ledsDmaRgbwInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsDmaRgbwInverted;
    ledsDmaRgbwInverted = nullptr;
  }
  if (ledsUartInverted != nullptr) {
    while (!ledsUartInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsUartInverted;
    ledsUartInverted = nullptr;
  }
  if (ledsUartRgbwInverted != nullptr) {
    while (!ledsUartRgbwInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsUartRgbwInverted;
    ledsUartRgbwInverted = nullptr;
  }
  if (ledsStandardInverted != nullptr) {
    while (!ledsStandardInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandardInverted;
    ledsStandardInverted = nullptr;
  }
  if (ledsStandardRgbwInverted != nullptr) {
    while (!ledsStandardRgbwInverted->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandardRgbwInverted;
    ledsStandardRgbwInverted = nullptr;
  }
#endif
  if (cleared) {
    Serial.println("LEDs cleared");
  }

}

void LedManager::cleanBgr() {

  boolean cleared = false;
#if defined(ESP32)
  if (ledsEsp32Bgr != NULL) {
    while (!ledsEsp32Bgr->CanShow()) { yield(); }
    cleared = true;
    delete ledsEsp32Bgr;
    ledsEsp32Bgr = NULL;
  }
#endif
#if defined(ESP8266)
  if (ledsDmaBgr != nullptr) {
    while (!ledsDmaBgr->CanShow()) { yield(); }
    cleared = true;
    delete ledsDmaBgr;
    ledsDmaBgr = nullptr;
  }
  if (ledsUartBgr != nullptr) {
    while (!ledsUartBgr->CanShow()) { yield(); }
    cleared = true;
    delete ledsUartBgr;
    ledsUartBgr = nullptr;
  }
  if (ledsStandardBgr != nullptr) {
    while (!ledsStandardBgr->CanShow()) { yield(); }
    cleared = true;
    delete ledsStandardBgr;
    ledsStandardBgr = nullptr;
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
  if (colorOrder == 1) {
    Serial.println(F("Using Standard"));
    cleanLEDs();
    ledsStandard = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandard == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsStandard->Begin();
    ledsStandard->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using Standard"));
    cleanLEDs();
    ledsStandardInverted = new NeoPixelBus<NeoRgbFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandardInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsStandardInverted->Begin();
    ledsStandardInverted->Show();
  } else if (colorOrder == 3) {
    Serial.println(F("Using Standard"));
    cleanLEDs();
    ledsStandardBgr = new NeoPixelBus<NeoBgrFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandardBgr == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsStandardBgr->Begin();
    ledsStandardBgr->Show();
  }
#endif

}

/**
 * Init led strip RGBW. No hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initStandardRgbw() {

#if defined(ESP8266)
  if (colorOrder == 1) {
    Serial.println(F("Using Standard RGBW"));
    cleanLEDs();
    ledsStandardRgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266BitBangSk6812Method >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandardRgbw == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));

    Serial.flush();
    ledsStandardRgbw->Begin();
    ledsStandardRgbw->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using Standard RGBW"));
    cleanLEDs();
    ledsStandardRgbwInverted = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266BitBangSk6812Method >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandardRgbwInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));

    Serial.flush();
    ledsStandardRgbwInverted->Begin();
    ledsStandardRgbwInverted->Show();
  }
#endif

}

/**
 * Init led strip RGB. Hardware UART, GPIO2, yes serial read/writeHardware UART, GPIO2, yes serial read/write
 */
void LedManager::initUart() {

#if defined(ESP8266)
  if (colorOrder == 1) {
    Serial.println(F("Using UART"));
    cleanLEDs();
    ledsUart = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUart == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsUart->Begin();
    ledsUart->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using UART"));
    cleanLEDs();
    ledsUartInverted = new NeoPixelBus<NeoRgbFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUartInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsUartInverted->Begin();
    ledsUartInverted->Show();
  } else if (colorOrder == 3) {
    Serial.println(F("Using UART"));
    cleanLEDs();
    ledsUartBgr = new NeoPixelBus<NeoBgrFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUartBgr == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsUartBgr->Begin();
    ledsUartBgr->Show();
  }
#endif

}

/**
 * Init led strip RGBW. Hardware UART, GPIO2, yes serial read/write
 */
void LedManager::initUartRgbw() {

#if defined(ESP8266)
  if (colorOrder == 1) {
    Serial.println(F("Using UART RGBW"));
    cleanLEDs();
    ledsUartRgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart1Sk6812Method>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUartRgbw == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsUartRgbw->Begin();
    ledsUartRgbw->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using UART RGBW"));
    cleanLEDs();
    ledsUartRgbwInverted = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Sk6812Method>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUartRgbwInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsUartRgbwInverted->Begin();
    ledsUartRgbwInverted->Show();
  }
#endif

}

/**
 * Init led strip RGB. Hardware DMA, GPIO3, no serial read, yes serial write
 */
void LedManager::initDma() {

#if defined(ESP8266)
  if (colorOrder == 1) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsDma = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
    if (ledsDma == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsDma->Begin();
    ledsDma->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsDmaInverted = new NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
    if (ledsDmaInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsDmaInverted->Begin();
    ledsDmaInverted->Show();
  } else if (colorOrder == 3) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsDmaBgr = new NeoPixelBus<NeoBgrFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
    if (ledsDmaBgr == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsDmaBgr->Begin();
    ledsDmaBgr->Show();
  }
#endif

}

/**
 * Init led strip RGBW. Hardware DMA, GPIO3, no serial read, yes serial write
 */
void LedManager::initDmaRgbw() {

#if defined(ESP8266)
  if (colorOrder == 1) {
    Serial.println(F("Using DMA RGBW"));
    cleanLEDs();
    ledsDmaRgbw = new NeoPixelBus<NeoGrbwFeature, NeoSk6812Method>(dynamicLedNum, 3); // and recreate with new count
    if (ledsDmaRgbw == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsDmaRgbw->Begin();
    ledsDmaRgbw->Show();
  } else if (colorOrder == 2) {
    Serial.println(F("Using DMA RGBW"));
    cleanLEDs();
    ledsDmaRgbwInverted = new NeoPixelBus<NeoRgbwFeature, NeoSk6812Method>(dynamicLedNum, 3); // and recreate with new count
    if (ledsDmaRgbwInverted == nullptr) {
      Serial.println(F("OUT OF MEMORY"));
    }
    Serial.println();
    Serial.println(F("Initializing..."));
    Serial.flush();
    ledsDmaRgbwInverted->Begin();
    ledsDmaRgbwInverted->Show();
  }
#endif

}

/**
 * Init led strip RGB. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32() {

#if defined(ESP32)
  if (colorOrder == 1) {
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
  } else if (colorOrder == 2) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsEsp32Inverted = new NeoPixelBus<NeoRgbFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum,gpioInUse); // and recreate with new count
    if (ledsEsp32Inverted == NULL) {
      Serial.println(F("OUT OF MEMORY"));
    } else {
      Serial.println();
      Serial.println(F("Initializing..."));
      Serial.flush();
      ledsEsp32Inverted->Begin();
      ledsEsp32Inverted->Show();
    }
  } else if (colorOrder == 3) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsEsp32Bgr = new NeoPixelBus<NeoBgrFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum,gpioInUse); // and recreate with new count
    if (ledsEsp32Bgr == NULL) {
      Serial.println(F("OUT OF MEMORY"));
    } else {
      Serial.println();
      Serial.println(F("Initializing..."));
      Serial.flush();
      ledsEsp32Bgr->Begin();
      ledsEsp32Bgr->Show();
    }
  }
#endif

}

/**
 * Init led strip RGBW. Hardware, ALL GPIO, yes serial read/write
 */
void LedManager::initEsp32Rgbw() {

#if defined(ESP32)
  if (colorOrder == 1) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsEsp32Rgbw = new NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Sk6812Method>(dynamicLedNum,gpioInUse); // and recreate with new count
    if (ledsEsp32Rgbw == NULL) {
      Serial.println(F("OUT OF MEMORY"));
    } else {
      Serial.println();
      Serial.println(F("Initializing..."));
      Serial.flush();
      ledsEsp32Rgbw->Begin();
      ledsEsp32Rgbw->Show();
    }
  } else if (colorOrder == 2) {
    Serial.println(F("Using DMA"));
    cleanLEDs();
    ledsEsp32RgbwInverted = new NeoPixelBus<NeoRgbwFeature, NeoEsp32I2s1Ws2812xMethod>(dynamicLedNum,gpioInUse); // and recreate with new count
    if (ledsEsp32RgbwInverted == NULL) {
      Serial.println(F("OUT OF MEMORY"));
    } else {
      Serial.println();
      Serial.println(F("Initializing..."));
      Serial.flush();
      ledsEsp32RgbwInverted->Begin();
      ledsEsp32RgbwInverted->Show();
    }
  }
#endif

}

/**
 * Init LEDs
 */
void LedManager::initLeds() {

  if ((colorOrder == 1) || (colorOrder == 2)) {
    switch (colorMode) {
      case 1:
        initEsp32();
        break;
      case 2:
      case 3:
      case 4:
        initEsp32Rgbw();
        break;
    }
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
      }
    }
  } else {
    switch (colorMode) {
      case 1:
      case 2:
      case 3:
      case 4:
        initEsp32();
        break;
    }
    if (gpioInUse == 3) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          initDma();
          break;
      }
    } else if (gpioInUse == 2) {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          initUart();
          break;
      }
    } else {
      switch (colorMode) {
        case 1:
        case 2:
        case 3:
        case 4:
          initStandard();
          break;
      }
    }
  }

}

/**
 * Set color mode received by the Firefly Luciferin software
 * @param colorMode
 */
void LedManager::setColorMode(int colorModeToUse) {

  Serial.println("CHANGING COLOR MODE");
  DynamicJsonDocument colorModeDoc(1024);
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
  DynamicJsonDocument colorOrderDoc(1024);
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
 */
void LedManager::setLdr(boolean ldrEnabledToSet, boolean ldrTurnOffToSet, uint8_t ldrIntervalToSet, uint8_t minLdr, boolean ledOn) {

  Serial.println(F("CHANGING LDR"));
  previousMillisLDR = 0;
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[LDR_PARAM] = ldrEnabledToSet;
  ldrDoc[LDR_TO_PARAM] = ldrTurnOffToSet;
  ldrDoc[LDR_INTER_PARAM] = ldrIntervalToSet;
  ldrDoc[MIN_LDR_PARAM] = minLdr;
  ldrDoc[LED_ON_PARAM] = ledOn;
  BootstrapManager::writeToLittleFS(ldrDoc, LDR_FILENAME);
  delay(20);

}


/**
 * Set LDR params received by the Firefly Luciferin software
 * @param maxLdr value used during the calibration
 */
void LedManager::setLdr(int maxLdr) {

  Serial.println(F("CHANGING LDR"));
  DynamicJsonDocument ldrDoc(1024);
  ldrDoc[MAX_LDR_PARAM] = maxLdr;
  BootstrapManager::writeToLittleFS(ldrDoc, LDR_CAL_FILENAME);
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
  DynamicJsonDocument numLedDoc(1024);
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
  DynamicJsonDocument whiteTempDoc(1024);
  whiteTempDoc[WHITE_TEMP_PARAM] = wt;
  BootstrapManager::writeToLittleFS(whiteTempDoc, WHITE_TEMP_FILENAME);
  delay(20);

}
