/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "GlowWormLuciferin.h"

/**
 * Setup function
 */
void setup() {
  firmwareVersion = VERSION;
  // if fastDisconnectionManagement we need to execute the disconnection callback immediately
  fastDisconnectionManagement = true;
  // BaudRate from configuration storage
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM);
  if (!baudRateFromStorage.isEmpty() && baudRateFromStorage != ERROR && baudRateFromStorage.toInt() != 0) {
    baudRateInUse = baudRateFromStorage.toInt();
  }
  int baudRateToUse = Globals::setBaudRateInUse(baudRateInUse);
  Serial.begin(baudRateToUse);
  while (!Serial); // wait for serial attach
  Serial.print(F("BAUDRATE IN USE="));
  Serial.println(baudRateToUse);
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  pinMode(SMART_BUTTON, INPUT_PULLUP);
#endif
  // LED number from configuration storage
  String ledNumToUse = bootstrapManager.readValueFromFile(LED_NUM_FILENAME, LED_NUM_PARAM);
  if (!ledNumToUse.isEmpty() && ledNumToUse != ERROR && ledNumToUse.toInt() != 0) {
    ledManager.dynamicLedNum = ledNumToUse.toInt();
  } else {
    ledManager.dynamicLedNum = 100;
  }
  Serial.print(F("\nUsing LEDs="));
  Serial.println(ledManager.dynamicLedNum);

  // White temp to use
  String whiteTempToUse = bootstrapManager.readValueFromFile(WHITE_TEMP_FILENAME, WHITE_TEMP_PARAM);
  if (!whiteTempToUse.isEmpty() && whiteTempToUse != ERROR && whiteTempToUse.toInt() != 0) {
    whiteTempInUse = whiteTempToUse.toInt();
  }
  Serial.print(F("\nUsing White temp="));
  Serial.println(whiteTempToUse);

#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
  MAC = WiFi.macAddress();
  bootstrapManager.littleFsInit();
  configureLeds();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  String ap = bootstrapManager.readValueFromFile(AP_FILENAME, AP_PARAM);
  if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 10) {
    setApState(11);
    ledManager.setColor(0, 255, 0);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 11) {
    setApState(12);
    ledManager.setColor(0, 0, 255);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 12) {
    bootstrapManager.littleFsInit();
    bootstrapManager.isWifiConfigured();
    setApState(13);
    ledManager.setColor(255, 75, 0);
    bootstrapManager.launchWebServerCustom(false, manageApRoot);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 13) {
    setApState(0);
  } else {
    bootstrapManager.littleFsInit();
    if (bootstrapManager.isWifiConfigured()) {
      configureLeds();
    }
  }
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // LED number from configuration storage
  String topicToUse = bootstrapManager.readValueFromFile(networkManager.TOPIC_FILENAME, networkManager.MQTT_PARAM);
  if (topicToUse != "null" && !topicToUse.isEmpty() && topicToUse != ERROR && topicToUse != networkManager.topicInUse) {
    networkManager.topicInUse = topicToUse;
    NetworkManager::executeMqttSwap(networkManager.topicInUse);
  }
  Serial.print(F("\nMQTT topic in use="));
  Serial.println(networkManager.topicInUse);

  // Bootsrap setup() with Wifi and MQTT functions
  bootstrapManager.bootstrapSetup(NetworkManager::manageDisconnections, NetworkManager::manageHardwareButton,
                                  NetworkManager::callback, true, manageApRoot);
#endif

  // Color mode from configuration storage
  String ldrFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_PARAM);
  String ldrTurnOffFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_TO_PARAM);
  String ldrIntervalFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_INTER_PARAM);
  String ldrMinFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.MIN_LDR_PARAM);
  String ldrMaxFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_CAL_FILENAME, ledManager.MAX_LDR_PARAM);
  String ledOnFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LED_ON_PARAM);
  if (!ldrFromStorage.isEmpty() && ldrFromStorage != ERROR) {
    ldrEnabled = ldrFromStorage == "1";
  }
  if (!ldrTurnOffFromStorage.isEmpty() && ldrTurnOffFromStorage != ERROR) {
    ldrTurnOff = ldrTurnOffFromStorage == "1";
  }
  if (!ldrIntervalFromStorage.isEmpty() && ldrIntervalFromStorage != ERROR) {
    ldrInterval = ldrIntervalFromStorage.toInt();
  }
  if (!ldrMinFromStorage.isEmpty() && ldrMinFromStorage != ERROR && ldrMinFromStorage.toInt() != 0) {
    ldrMin = ldrMinFromStorage.toInt();
  }
  if (!ldrMaxFromStorage.isEmpty() && ldrMaxFromStorage != ERROR && ldrMaxFromStorage.toInt() != 0) {
    ldrDivider = ldrMaxFromStorage.toInt();
    if (ldrDivider == -1) {
      ldrDivider = LDR_DIVIDER;
    }
  }
  String r = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("r"));
  if (!r.isEmpty() && r != ERROR && r.toInt() != -1) {
    ledManager.red = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("r")).toInt();
    rStored = ledManager.red;
    ledManager.green = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("g")).toInt();
    gStored = ledManager.green;
    ledManager.blue = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("b")).toInt();
    bStored = ledManager.blue;
    brightness = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("brightness")).toInt();
    brightnessStored = brightness;
  }
  String as = bootstrapManager.readValueFromFile(AUTO_SAVE_FILENAME, F("autosave"));
  if (!as.isEmpty() && r != ERROR && as.toInt() != -1) {
    autoSave = bootstrapManager.readValueFromFile(AUTO_SAVE_FILENAME, F("autosave")).toInt();
  }

#if defined(ESP8266)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
#elif defined(ESP32)
  pinMode(RELAY_PIN_DIG, OUTPUT);
  digitalWrite(RELAY_PIN_DIG, LOW);
  pinMode(RELAY_PIN_PICO, OUTPUT);
  digitalWrite(RELAY_PIN_PICO, LOW);
#endif

#if defined(ESP32)
  esp_task_wdt_init(3000, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
#endif
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Begin listening to UDP port
  networkManager.UDP.begin(UDP_PORT);
  networkManager.broadcastUDP.begin(UDP_BROADCAST_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);
  NetworkManager::fpsData.reserve(200);
  networkManager.prefsData.reserve(200);
  networkManager.listenOnHttpGet();
#if defined(ESP8266)
  // Hey gateway, GlowWorm is here
  delay(DELAY_500);
  pingESP.ping();
#endif
#endif
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (!ledOnFromStorage.isEmpty() && ledOnFromStorage != ERROR) {
    ledOn = ledOnFromStorage == "1";
    if (ledOn) {
      Globals::turnOnRelay();
      ledManager.stateOn = true;
      effect = Effect::solid;
      networkManager.setColor();
    }
  }
#endif
}

/**
 * Configure LEDs using the stored params
 */
void configureLeds() {
  // GPIO pin from configuration storage, overwrite the one saved during initial Arduino Bootstrapper config
  String gpioFromStorage = bootstrapManager.readValueFromFile(GPIO_FILENAME, GPIO_PARAM);
  int gpioToUse = 0;
  if (!gpioFromStorage.isEmpty() && gpioFromStorage != ERROR && gpioFromStorage.toInt() != 0) {
    gpioToUse = gpioFromStorage.toInt();
  }
  if (gpioToUse == 0) {
    if (!additionalParam.isEmpty()) {
      gpioToUse = additionalParam.toInt();
    }
  }
  switch (gpioToUse) {
    case 5:
      gpioInUse = 5;
      break;
    case 3:
      gpioInUse = 3;
      break;
    case 16:
      gpioInUse = 16;
      break;
    default:
      gpioInUse = 2;
      break;
  }
  Serial.print(F("GPIO IN USE="));
  Serial.println(gpioInUse);

  // Color mode from configuration storage
  String colorModeFromStorage = bootstrapManager.readValueFromFile(ledManager.COLOR_MODE_FILENAME,
                                                                   ledManager.COLOR_MODE_PARAM);
  if (!colorModeFromStorage.isEmpty() && colorModeFromStorage != ERROR && colorModeFromStorage.toInt() != 0) {
    colorMode = colorModeFromStorage.toInt();
  }
  Serial.print(F("COLOR_MODE IN USE="));
  Serial.println(colorMode);

  // Color order from configuration storage
  String colorOrderFromStorage = bootstrapManager.readValueFromFile(ledManager.COLOR_ORDER_FILENAME,
                                                                    ledManager.COLOR_ORDER_PARAM);
  if (!colorOrderFromStorage.isEmpty() && colorOrderFromStorage != ERROR && colorOrderFromStorage.toInt() != 0) {
    colorOrder = colorOrderFromStorage.toInt();
  }
  Serial.print(F("COLOR_ORDER IN USE="));
  Serial.println(colorOrder);

  ledManager.initLeds();

}

/**
 * Read serial or break the reading
 * @return -1 if loop must break
 */
int serialRead() {

  return !breakLoop ? Serial.read() : -1;

}

#ifdef TARGET_GLOWWORMLUCIFERINFULL

void manageApRoot() {
  networkManager.manageAPSetting(true);
}

void setApState(byte state) {
  configureLeds();
  DynamicJsonDocument asDoc(1024);
  asDoc[AP_PARAM] = state;
  BootstrapManager::writeToLittleFS(asDoc, AP_FILENAME);
  effect = Effect::solid;
  ledManager.stateOn = true;
}

#endif

/**
 * Main loop
 */
void mainLoop() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  NetworkManager::checkConnection();
#endif

  // GLOW_WORM_LUCIFERIN, serial connection with Firefly Luciferin
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (effect == Effect::GlowWorm) {
#endif
    if (!ledManager.led_state) ledManager.led_state = true;
    int loopIdx;
    for (loopIdx = 0; loopIdx < prefixLength; ++loopIdx) {
      waitLoop:
      while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
      if (breakLoop || prefix[loopIdx] == serialRead()) continue;
      loopIdx = 0;
      goto waitLoop;
    }
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    hi = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    lo = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    loSecondPart = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    usbBrightness = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    gpio = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    baudRate = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    whiteTemp = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    fireflyEffect = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    ldrEn = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    ldrTo = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    ldrInt = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    ldrMn = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    ldrAction = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    fireflyColorMode = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    fireflyColorOrder = serialRead();
    while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
    chk = serialRead();
    if (!breakLoop && (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ whiteTemp ^ fireflyEffect
                               ^ ldrEn ^ ldrTo ^ ldrInt ^ ldrMn ^ ldrAction ^ fireflyColorMode ^ fireflyColorOrder ^ 0x55))) {
      loopIdx = 0;
      goto waitLoop;
    }
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    if (!relayState) {
    Globals::turnOnRelay();
  }
#endif
    if ((usbBrightness != brightness) & !ldrEnabled) {
      brightness = usbBrightness;
    }
    if (gpio != 0 && gpioInUse != gpio && (gpio == 2 || gpio == 3 || gpio == 5 || gpio == 16)) {
      Globals::setGpio(gpio);
      ledManager.reinitLEDTriggered = true;
    }
    if (ldrAction == 2 || ldrAction == 3 || ldrAction == 4) {
      ldrEnabled = ldrEn == 1;
      ldrTurnOff = ldrTo == 1;
      ldrInterval = ldrInt;
      ldrMin = ldrMn;
      ledManager.setLdr(ldrEn == 1, ldrTo == 1, ldrInt, ldrMn, ledOn);
      delay(DELAY_500);
      if (ldrAction == 2) {
        ldrDivider = ldrValue;
        ledManager.setLdr(ldrDivider);
      } else if (ldrAction == 3) {
        ldrDivider = LDR_DIVIDER;
        ledManager.setLdr(-1);
      }
    }
    uint16_t numLedFromLuciferin = lo + loSecondPart + 1;
    if ((ledManager.dynamicLedNum != numLedFromLuciferin) && (numLedFromLuciferin < NUM_LEDS)) {
      LedManager::setNumLed(numLedFromLuciferin);
      ledManager.reinitLEDTriggered = true;
    }
    if (ledManager.reinitLEDTriggered) {
      ledManager.reinitLEDTriggered = false;
      ledManager.initLeds();
      breakLoop = true;
    }
    if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 8)) {
      Globals::setBaudRate(baudRate);
#if defined(ESP32)
      ESP.restart();
#elif defined(ESP8266)
      EspClass::restart();
#endif
    }
    if (whiteTemp != 0 && whiteTempInUse != whiteTemp && (whiteTemp >= 20 && whiteTemp <= 110)) {
      LedManager::setWhiteTemp(whiteTemp);
    }
    // If MQTT is enabled but using USB cable, effect is 0 and is set via MQTT callback
    if (fireflyEffect != 0 && ledManager.fireflyEffectInUse != fireflyEffect) {
      ledManager.fireflyEffectInUse = fireflyEffect;
      switch (ledManager.fireflyEffectInUse) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
        case 1:
      case 2:
      case 3:
      case 4:
      case 5:
          if (effect != Effect::GlowWorm) {
            previousMillisLDR = 0;
          }
          effect = Effect::GlowWorm;
          break;
#endif
        case 6:
          effect = Effect::solid;
          break;
        case 7:
          effect = Effect::fire;
          break;
        case 8:
          effect = Effect::twinkle;
          break;
        case 9:
          effect = Effect::bpm;
          break;
        case 10:
          effect = Effect::mixed_rainbow;
          break;
        case 11:
          effect = Effect::rainbow;
          break;
        case 12:
          effect = Effect::chase_rainbow;
          break;
        case 13:
          effect = Effect::solid_rainbow;
          break;
        case 100:
          ledManager.fireflyEffectInUse = 0;
          break;
      }
    }
    if (fireflyColorMode != 0 && (fireflyColorMode >= 1 && fireflyColorMode <= 4)) {
      ledManager.setColorModeInit(fireflyColorMode);
    }
    if (fireflyColorOrder != 0 && (fireflyColorOrder >= 1 && fireflyColorOrder <= 3)) {
      ledManager.setColorOrderInit(fireflyColorOrder);
    }
    // memset(leds, 0, (numLedFromLuciferin) * sizeof(struct CRGB));
    // Serial.readBytes( (char*)leds, numLedFromLuciferin * 3);
    for (uint16_t i = 0; i < (numLedFromLuciferin); i++) {
      byte r, g, b;
      while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
      r = serialRead();
      while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
      g = serialRead();
      while (!breakLoop && !Serial.available()) NetworkManager::checkConnection();
      b = serialRead();
      if (ldrInterval != 0 && ldrEnabled && ldrReading && ldrTurnOff) {
        r = g = b = 0;
      }
      if (ledManager.fireflyEffectInUse <= 6) {
        ledManager.setPixelColor(i, r, g, b);
      }
    }
    ledManager.lastLedUpdate = millis();
    framerateCounter++;
    ledManager.ledShow();

    // Flush serial buffer
    while (!breakLoop && Serial.available() > 0) {
      serialRead();
    }
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  }
#endif

  if (effect == Effect::GlowWorm || effect == Effect::GlowWormWifi) {
    temporaryDisableImprove = true;
  } else {
    temporaryDisableImprove = false;
  }

  breakLoop = false;

  //EFFECT BPM
  if (effect == Effect::bpm) {
    effectsManager.bpm(currentPalette, targetPalette);
  }

  //EFFECT RAINBOW
  if (effect == Effect::rainbow) {
    effectsManager.rainbow(ledManager.dynamicLedNum);
  }

  //SOLID RAINBOW
  if (effect == Effect::solid_rainbow) {
    effectsManager.solidRainbow(ledManager.dynamicLedNum);
  }

  //FIRE
  if (effect == Effect::fire) {
    EffectsManager::fire(55, 120, 15, ledManager.dynamicLedNum);
  }

  //TWINKLE
  if (effect == Effect::twinkle) {
    EffectsManager::twinkleRandom(20, 100, false, ledManager.dynamicLedNum);
  }

  //CHASE RAINBOW
  if (effect == Effect::chase_rainbow) {
    EffectsManager::theaterChaseRainbow(ledManager.dynamicLedNum);
  }

  //MIXED RAINBOW
  if (effect == Effect::mixed_rainbow) {
    effectsManager.mixedRainbow(ledManager.dynamicLedNum);
  }

}

/**
 * Loop
 */
void loop() {

  mainLoop();

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  btnState = digitalRead(SMART_BUTTON);
  if (lastState == HIGH && btnState == LOW) {
    pressedTime = millis();
  } else if (lastState == LOW && btnState == HIGH) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;
    if ((pressDuration > DEBOUNCE_PRESS_TIME) && (pressedTime > 0) && (pressDuration < SHORT_PRESS_TIME)) {
      if (!ledManager.stateOn) {
        Globals::turnOnRelay();
        ledManager.stateOn = true;
        networkManager.setColor();
      } else {
        ledManager.stateOn = false;
        networkManager.setColor();
        Globals::turnOffRelay();
      }
    }
  }
  lastState = btnState;

  if (!apFileRead) {
    apFileRead = true;
    String ap = bootstrapManager.readValueFromFile(AP_FILENAME, AP_PARAM);
    if (!ap.isEmpty() && ap != ERROR && ap.toInt() != 0) {
      setApState(0);
      ledManager.setColor(0, 0, 0);
    }
    disconnectionCounter = 0;
  }
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (relayState && !ledManager.stateOn) {
    Globals::turnOffRelay();
  }
  networkManager.getUDPStream();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
#if defined(ESP8266)
  EVERY_N_SECONDS(30) {
    // Hey gateway, GlowWorm is here
    if (!pingESP.ping()) {
      WiFi.reconnect();
    }
  }
#endif
#endif
#if defined(ESP32)
  EVERY_N_MILLISECONDS(3000) {
    esp_task_wdt_reset();
  }
#endif

  if (ldrEnabled) {
    if (ldrInterval == 0) {
      EVERY_N_SECONDS(1) {
        ldrReading = true;
        previousMillisLDR = millis();
      }
    } else {
      if (((millis() - previousMillisLDR) >= ((ldrInterval * 1000) * 60)) || previousMillisLDR == 0) {
        ldrReading = true;
        previousMillisLDR = millis();
      }
    }
    if (ldrReading) {
      if ((ldrInterval == 0) || ((millis() - previousMillisLDR) >= LDR_RECOVER_TIME)) {
#if defined(ESP8266)
        ldrValue = analogRead(LDR_PIN);
#elif defined(ESP32)
        int tmpLdrVal = analogRead(LDR_PIN_DIG);
        ldrValue = analogRead(LDR_PIN_PICO);
        if (tmpLdrVal > ldrValue) ldrValue = tmpLdrVal;
#endif
        uint8_t minBright = (ldrMin * 255) / 100;
        int br = ((((ldrValue * 100) / ldrDivider) * 255) / 100);
        if (br > 255) {
          brightness = 255;
        } else {
          brightness = br;
        }
        if (brightness <= minBright) {
          brightness = minBright;
        }
        ldrReading = false;
      }
    }
  }

}