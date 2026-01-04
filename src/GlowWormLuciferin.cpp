/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright © 2020 - 2026  Davide Perini

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

#include <FS.h> // this needs to be first, or it all crashes and burns...
#include "GlowWormLuciferin.h"

/**
 * Setup function
 */
void setup() {
  String ledBiFromStorage = bootstrapManager.readValueFromFile(ledManager.PIN_FILENAME, ledManager.LED_BUILTIN_PARAM);
  if (!ledBiFromStorage.isEmpty() && ledBiFromStorage != ERROR) {
    ledBuiltin = ledBiFromStorage.toInt();
  }

  LedManager::manageBuiltInLed(0, 0, 255);
  firmwareVersion = VERSION;
  // if fastDisconnectionManagement we need to execute the disconnection callback immediately
  fastDisconnectionManagement = true;

  // BaudRate from configuration storage
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM);
  if (!baudRateFromStorage.isEmpty() && baudRateFromStorage != ERROR && baudRateFromStorage.toInt() != 0) {
    baudRateInUse = baudRateFromStorage.toInt();
  }

  int baudRateToUse = Globals::setBaudRateInUse(baudRateInUse);

#if defined(ARDUINO_ARCH_ESP32)
  // Increase the RX Buffer size allows to send bigger messages via Serial in one chunk, increase performance.
  Serial.setRxBufferSize(SERIAL_SIZE_RX);
#endif

  Serial.begin(baudRateToUse);
  Serial.setTimeout(10);
  Serial.setDebugOutput(false); // switch off kernel messages when using USBCDC

#if CONFIG_IDF_TARGET_ESP32 || defined(ESP8266)
  while (!Serial); // wait for serial attach
#endif

  Serial.print(F("BAUDRATE IN USE="));
  Serial.println(baudRateToUse);

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  pinMode(sbPin, INPUT_PULLUP);
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
    LedManager::setColorLoop(0, 255, 0);
    LedManager::manageBuiltInLed(0, 255, 0);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 11) {
    setApState(12);
    LedManager::setColorLoop(0, 0, 255);
    LedManager::manageBuiltInLed(0, 0, 255);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 12) {
    bootstrapManager.littleFsInit();
    BootstrapManager::isWifiConfigured();
    setApState(13);
    LedManager::setColorLoop(255, 75, 0);
    LedManager::manageBuiltInLed(255, 75, 0);
    bootstrapManager.launchWebServerCustom(false, manageApRoot);
  } else if (!ap.isEmpty() && ap != ERROR && ap.toInt() == 13) {
    setApState(0);
  } else {
    bootstrapManager.littleFsInit();
    if (BootstrapManager::isWifiConfigured()) {
      configureLeds();
    }
  }
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // LED number from configuration storage
  String topicToUse = bootstrapManager.readValueFromFile(netManager.TOPIC_FILENAME, netManager.MQTT_PARAM);
  if (topicToUse != "null" && !topicToUse.isEmpty() && topicToUse != ERROR && topicToUse != netManager.topicInUse) {
    netManager.topicInUse = topicToUse;
    NetManager::executeMqttSwap(netManager.topicInUse);
  }
  Serial.print(F("\nMQTT topic in use="));
  Serial.println(netManager.topicInUse);

  // Bootsrap setup() with Wifi and MQTT functions
  blockingMqtt = false;
  bootstrapManager.bootstrapSetup(NetManager::manageDisconnections, NetManager::manageHardwareButton,
                                  NetManager::callback, true, manageApRoot);
#endif

  // Color mode from configuration storage
  String ldrFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_PARAM);
  String ldrTurnOffFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_TO_PARAM);
  String ldrIntervalFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.LDR_INTER_PARAM);
  String ldrMinFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_FILENAME, ledManager.MIN_LDR_PARAM);
  String ldrMaxFromStorage = bootstrapManager.readValueFromFile(ledManager.LDR_CAL_FILENAME, ledManager.MAX_LDR_PARAM);
  String relayPinFromStorage = bootstrapManager.readValueFromFile(ledManager.PIN_FILENAME, ledManager.RELAY_PIN_PARAM);
  String relayInvStorage = bootstrapManager.readValueFromFile(ledManager.PIN_FILENAME, ledManager.RELAY_INV);
  String sbPinFromStorage = bootstrapManager.readValueFromFile(ledManager.PIN_FILENAME, ledManager.SB_PIN_PARAM);
  String ldrPinFromStorage = bootstrapManager.readValueFromFile(ledManager.PIN_FILENAME, ledManager.LDR_PIN_PARAM);

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
  if (!relayPinFromStorage.isEmpty() && relayPinFromStorage != ERROR) {
    relayPin = relayPinFromStorage.toInt();
  }
  if (!relayInvStorage.isEmpty() && relayInvStorage != ERROR) {
    relInv = relayInvStorage.toInt();
  }
  if (!sbPinFromStorage.isEmpty() && sbPinFromStorage != ERROR) {
    sbPin = sbPinFromStorage.toInt();
  }
  if (!ldrPinFromStorage.isEmpty() && ldrPinFromStorage != ERROR) {
    ldrPin = ldrPinFromStorage.toInt();
  }
  if (!ldrMaxFromStorage.isEmpty() && ldrMaxFromStorage != ERROR && ldrMaxFromStorage.toInt() != 0) {
    ldrDivider = ldrMaxFromStorage.toInt();
    if (ldrDivider == -1) {
      ldrDivider = LDR_DIVIDER;
    }
  }

  String r = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("r"));
  String ef = Globals::effectToString(Effect::solid);
  if (!r.isEmpty() && r != ERROR && r.toInt() != -1) {
    ledManager.red = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("r")).toInt();
    rStored = ledManager.red;
    ledManager.green = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("g")).toInt();
    gStored = ledManager.green;
    ledManager.blue = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("b")).toInt();
    bStored = ledManager.blue;
    brightness = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("brightness")).toInt();
    brightnessStored = brightness;
    ef = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("effect"));
    effectStored = Globals::stringToEffect(ef);
    toggleStored = bootstrapManager.readValueFromFile(COLOR_BRIGHT_FILENAME, F("toggle")).toInt();
  }

  String as = bootstrapManager.readValueFromFile(AUTO_SAVE_FILENAME, F("autosave"));
  if (!as.isEmpty() && r != ERROR && as.toInt() != -1) {
    autoSave = bootstrapManager.readValueFromFile(AUTO_SAVE_FILENAME, F("autosave")).toInt();
  }

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Begin listening to UDP port
  netManager.UDP.begin(UDP_PORT);
  netManager.broadcastUDP.begin(UDP_BROADCAST_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);
  NetManager::fpsData.reserve(200);
  netManager.prefsData.reserve(200);
  netManager.listenOnHttpGet();

#if defined(ESP8266)
  // Hey gateway, GlowWorm is here
  delay(DELAY_500);
  pingESP.ping();
#endif
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (toggleStored) {
    Globals::turnOnRelay();
    ledManager.stateOn = true;
    effect = Globals::stringToEffect(ef);
    NetManager::setColor();
  }
#endif

  LedManager::manageBuiltInLed(0, 0, 0);

#if defined(ARDUINO_ARCH_ESP32)
  xTaskCreatePinnedToCore(ldrTask, "ldr", 2048, NULL, 1, NULL, 0);
#endif
}

/**
 * LDR Task (ESP32 FreeRTOS)
 */
#if defined(ARDUINO_ARCH_ESP32)
void ldrTask(void*) {
  for (;;) {
    manageLdr();
    vTaskDelay(10);
  }
}
#endif

/**
 * Configure LEDs using the stored params
 */
void configureLeds() {
  // GPIO pin from configuration storage, overwrite the one saved during initial Arduino Bootstrapper config
  String gpioFromStorage = bootstrapManager.readValueFromFile(GPIO_FILENAME, GPIO_PARAM);
  if (!gpioFromStorage.isEmpty() && gpioFromStorage != ERROR && gpioFromStorage.toInt() != 0) {
    gpioInUse = gpioFromStorage.toInt();
#if defined(ESP8266)
    if (LED_BUILTIN != gpioInUse) {
      pinMode(LED_BUILTIN, OUTPUT);
    }
#endif
  }

  Serial.print(F("GPIO IN USE="));
  Serial.println(gpioInUse);

  // GPIO clock pin from configuration storage
  String gpioClockFromStorage = bootstrapManager.readValueFromFile(GPIO_CLOCK_FILENAME, GPIO_CLOCK_PARAM);
  if (!gpioClockFromStorage.isEmpty() && gpioClockFromStorage != ERROR && gpioClockFromStorage.toInt() != 0) {
    gpioClockInUse = gpioClockFromStorage.toInt();
  }

  Serial.print(F("GPIO CLOCK IN USE="));
  Serial.println(gpioClockInUse);

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
  netManager.manageAPSetting(true);
}

void setApState(byte state) {
  configureLeds();
  JsonDocument asDoc;
  asDoc[AP_PARAM] = state;
  BootstrapManager::writeToLittleFS(asDoc, AP_FILENAME);
  effect = Effect::solid;
  ledManager.stateOn = true;
}

#endif

/**
 * Main loop (workhorse)
 */
void mainLoop() {
  yield();
  NetManager::checkConnection();

  // GLOW_WORM_LUCIFERIN, serial connection with Firefly Luciferin
  if (Serial.peek() != -1) { // Using peek() instead of available() because it's non blocking
#ifdef TARGET_GLOWWORMLUCIFERINFULL
    if (effect == Effect::GlowWorm) {
#endif
      if (!ledManager.led_state) ledManager.led_state = true;

      int i = 0;
      yield();
      int prefixLength = Serial.readBytes((byte *) pre, CONFIG_PREFIX_LENGTH);
      bool prefixOk = false;
      if (prefixLength == CONFIG_PREFIX_LENGTH) {
        if (pre[0] == prefix[0] && pre[1] == prefix[1] && pre[2] == prefix[2] && pre[3] == prefix[3] &&
            pre[4] == prefix[4] && pre[5] == prefix[5]) {
          prefixOk = true;
        }
      }

      if (prefixOk) {
        yield();
        int configLen = Serial.readBytes((byte *) config, CONFIG_NUM_PARAMS);
        if (configLen == CONFIG_NUM_PARAMS) {
          hi = config[i++];
          lo = config[i++];
          loSecondPart = config[i++];
          usbBrightness = config[i++];
          gpio = config[i++];
          baudRate = config[i++];
          whiteTemp = config[i++];
          fireflyEffect = config[i++];
          ldrEn = config[i++];
          ldrTo = config[i++];
          ldrInt = config[i++];
          ldrMn = config[i++];
          ldrAction = config[i++];
          fireflyColorMode = config[i++];
          fireflyColorOrder = config[i++];
          relaySerialPin = config[i++];
          relayInvPin = config[i++];
          sbSerialPin = config[i++];
          ldrSerialPin = config[i++];
          gpioClock = config[i++];
          chk = config[i++];

          if (!(!breakLoop &&
                (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ whiteTemp ^ fireflyEffect
                         ^ ldrEn ^ ldrTo ^ ldrInt ^ ldrMn ^ ldrAction ^ fireflyColorMode ^ fireflyColorOrder
                         ^ relaySerialPin ^ relayInvPin ^ sbSerialPin ^ ldrSerialPin ^ gpioClock ^ 0x55)))) {
            if (!breakLoop) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
              if (!relayState) {
                Globals::turnOnRelay();
              }
#endif

              if ((usbBrightness != brightness) & !ldrEnabled) {
                brightness = usbBrightness;
              }

              if (gpio != 0 && gpioInUse != gpio) {
                Globals::setGpio(gpio);
                ledManager.reinitLEDTriggered = true;
              }

              if (gpioClock != 0 && gpioClockInUse != gpioClock) {
                Globals::setGpioClock(gpioClock);
                ledManager.reinitLEDTriggered = true;
              }

              if (ldrAction == 2 || ldrAction == 3 || ldrAction == 4) {
                ldrEnabled = ldrEn == 1;
                ldrTurnOff = ldrTo == 1;
                ldrInterval = ldrInt;
                ldrMin = ldrMn;
                ledManager.setLdr(ldrEn == 1, ldrTo == 1, ldrInt, ldrMn);
                delay(DELAY_500);
                if (ldrAction == 2) {
                  ldrDivider = ldrValue;
                  ledManager.setLdr(ldrDivider);
                } else if (ldrAction == 3) {
                  ldrDivider = LDR_DIVIDER;
                  ledManager.setLdr(-1);
                }
              }

              // Pins is set to +10 because null values are zero, so GPIO 0 is 10, GPIO 1 is 11.
              if (relaySerialPin > 9 && sbSerialPin > 9 && ldrSerialPin > 9) {
                relaySerialPin = relaySerialPin - 10;
                sbSerialPin = sbSerialPin - 10;
                ldrSerialPin = ldrSerialPin - 10;

                if ((relayPin != relaySerialPin) || (sbPin != sbSerialPin) || (ldrPin != ldrSerialPin) || (relayInvPin == 10 && relInv) || (relayInvPin == 11 && !relInv)) {
                  relayPin = relaySerialPin;
                  sbPin = sbSerialPin;
                  ldrPin = ldrSerialPin;
                  relInv = relayInvPin == 11;
                  ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
                }
              }

              uint16_t numLedFromLuciferin = lo + (loSecondPart * SERIAL_CHUNK_SIZE) + 1;
              if (ledManager.dynamicLedNum != numLedFromLuciferin) {
                LedManager::setNumLed(numLedFromLuciferin);
                ledManager.reinitLEDTriggered = true;
              }

              if (ledManager.reinitLEDTriggered) {
                ledManager.reinitLEDTriggered = false;
                ledManager.initLeds();
                breakLoop = true;
              }

              if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 10)) {
                Globals::setBaudRate(baudRate);
#if defined(ARDUINO_ARCH_ESP32)
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
                    effect = Effect::rainbow;
                    break;
                  case 11:
                    effect = Effect::slowRainbow;
                    break;
                  case 12:
                    effect = Effect::chase_rainbow;
                    break;
                  case 13:
                    effect = Effect::solid_rainbow;
                    break;
                  case 14:
                    effect = Effect::randomColors;
                    break;
                  case 15:
                    effect = Effect::rainbowColors;
                    break;
                  case 16:
                    effect = Effect::meteor;
                    break;
                  case 17:
                    effect = Effect::colorWaterfall;
                    break;
                  case 18:
                    effect = Effect::randomMarquee;
                    break;
                  case 19:
                    effect = Effect::rainbowMarquee;
                    break;
                  case 20:
                    effect = Effect::pulsing_rainbow;
                    break;
                  case 21:
                    effect = Effect::christmas;
                    break;
                  case 100:
                    ledManager.fireflyEffectInUse = 0;
                    break;
                }
              }

              if (fireflyColorMode != 0 && (fireflyColorMode >= 1 && fireflyColorMode <= 5)) {
                ledManager.setColorModeInit(fireflyColorMode);
              }

              if (fireflyColorOrder != 0 && (fireflyColorOrder >= 1 && fireflyColorOrder <= 6)) {
                ledManager.setColorOrderInit(fireflyColorOrder);
              }

              int rlenChunk;
              if ((numLedFromLuciferin * 3) < LED_BUFF) {
                rlenChunk = numLedFromLuciferin * 3;
              } else {
                rlenChunk = LED_BUFF;
              }

              // Serial buffer is read with a single block using Serial.readBytes()
              yield();
              int rlen = Serial.readBytes((byte *) ledBuffer, rlenChunk);
              if (rlenChunk == rlen) {
                i = 0;
                int j = 0;
                while (i < rlen) {
                  byte r, g, b;
                  r = ledBuffer[i++];
                  g = ledBuffer[i++];
                  b = ledBuffer[i++];
                  setSerialPixel(j, r, g, b);
                  j++;
                }

                // If there are many LEDs and buffer is too small, read the first block with Serial.readBytes() and then continue with Serial.read()
                while (j < numLedFromLuciferin) {
                  byte r, g, b;
                  while (!breakLoop && !Serial.available()) NetManager::checkConnection();
                  r = serialRead();
                  while (!breakLoop && !Serial.available()) NetManager::checkConnection();
                  g = serialRead();
                  while (!breakLoop && !Serial.available()) NetManager::checkConnection();
                  b = serialRead();
                  setSerialPixel(j, r, g, b);
                  j++;
                }

                ledManager.lastLedUpdate = millis();
                framerateCounterSerial++;
                ledManager.ledShow();
              }
            }
          }
        }
      }
#ifdef TARGET_GLOWWORMLUCIFERINFULL
    }
#endif
  }

  if (effect == Effect::solid && !ledManager.transitioning) {
    temporaryDisableImprove = false;
  } else {
    temporaryDisableImprove = true;
  }

  breakLoop = false;

  // Effects dispatch
  if (effect == Effect::bpm) {
    effectsManager.bpm();
  } else if (effect == Effect::fire) {
    effectsManager.fire(55, 120, 15);
  } else if (effect == Effect::rainbow) {
    effectsManager.rainbow(false);
  } else if (effect == Effect::slowRainbow) {
    effectsManager.rainbow(true);
  } else if (effect == Effect::solid_rainbow) {
    effectsManager.solidRainbow();
  } else if (effect == Effect::twinkle) {
    EffectsManager::twinkleRandom();
  } else if (effect == Effect::chase_rainbow) {
    effectsManager.theaterChaseRainbow();
  } else if (effect == Effect::randomColors) {
    EffectsManager::randomColors();
  } else if (effect == Effect::rainbowColors) {
    EffectsManager::rainbowColors();
  } else if (effect == Effect::meteor) {
    EffectsManager::meteor();
  } else if (effect == Effect::colorWaterfall) {
    EffectsManager::colorWaterfall();
  } else if (effect == Effect::randomMarquee) {
    EffectsManager::randomMarquee();
  } else if (effect == Effect::rainbowMarquee) {
    EffectsManager::rainbowMarquee();
  } else if (effect == Effect::pulsing_rainbow) {
    EffectsManager::pulsing_rainbow();
  } else if (effect == Effect::christmas) {
    EffectsManager::christmas();
  }
}

// Helper used by serial stream processing
void setSerialPixel(int j, byte r, byte g, byte b) {
  if (ldrInterval != 0 && ldrEnabled && ldrReading && ldrTurnOff) {
    r = g = b = 0;
  }
  if (ledManager.fireflyEffectInUse <= 6) {
    ledManager.setPixelColor(j, r, g, b);
  }
}

#ifdef TARGET_GLOWWORMLUCIFERINFULL
void debounceSmartButton() {
  int reading = digitalRead(sbPin);

  if (reading != lastButtonState) {
    lastDebounceTime = currentMillisMainLoop;
  }

  if ((currentMillisMainLoop - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // LOW = premuto (INPUT_PULLUP)
      if (buttonState == LOW) {
#if defined(ARDUINO_ARCH_ESP32)
        if (currentMillisMainLoop > esp32DebouceInitialPeriod) {
#else
        if (currentMillisMainLoop > esp8266DebouceInitialPeriod) {
#endif
          if (!ledManager.stateOn) {
            Globals::turnOnRelay();
            ledManager.stateOn = true;
            NetManager::setColor();
          } else {
            ledManager.stateOn = false;
            NetManager::setColor();
            Globals::turnOffRelay();
          }
        }
      }
    }
  }

  lastButtonState = reading;
}

#endif

/**
 * LDR manager
 * - This is a robust, non-shadowing implementation suitable for ESP8266 and ESP32.
 * - Key points:
 *   * Do NOT redeclare globals here (previousMillisLDR, ldrReading, ldrValue, brightness, ...)
 *   * Use yield() on ESP8266 to avoid watchdog resets on long blocking operations
 *   * Keep analogRead() calls minimised and guarded by timing logic
 */
void manageLdr() {
  // if LDR disabled, nothing to do
  if (!ldrEnabled) return;

  // Ensure currentMillisMainLoop is up-to-date when called from elsewhere
  // (loop() already updates it; if invoked from a task, consider setting it here too)

  // Case 1: interval == 0 -> sample every DELAY_1000 (1s)
  if (ldrInterval == 0) {
    if ((currentMillisMainLoop - previousMillisLDR) > DELAY_1000) {
      // mark start of reading time
      previousMillisLDR = currentMillisMainLoop;
      ldrReading = true;
    }
  } else {
    // interval in minutes, convert to ms
    unsigned long intervalMs = (unsigned long)ldrInterval * 60UL * 1000UL;
    if (previousMillisLDR == 0 || (currentMillisMainLoop - previousMillisLDR) >= intervalMs) {
      ldrReading = true;
      previousMillisLDR = currentMillisMainLoop;
    }
  }

  if (!ldrReading) return;

  // If we recently read (recover time), skip reading to give LDR/ADC time
  if ((ldrInterval == 0) || ((currentMillisMainLoop - previousMillisLDR) >= LDR_RECOVER_TIME)) {
    // On ESP8266 long ADC operations can trigger watchdog, so yield before/after
#if defined(ESP8266)
    yield();
#endif
    // Read ADC
    int raw = analogRead(ldrPin);
#if defined(ESP8266)
    yield();
#endif

    // Store the reading in the global ldrValue (do not create a local with same name)
    ldrValue = raw;

    // Calculate brightness scaled 0..255 using ldrDivider and ldrMin
    uint8_t minBright = (uint8_t)((ldrMin * 255) / 100);
    int br = 0;
    if (ldrDivider != 0) {
      br = ((((ldrValue * 100) / ldrDivider) * 255) / 100);
    }

    if (br > 255) {
      brightness = 255;
    } else if (br <= minBright) {
      brightness = minBright;
    } else {
      brightness = (uint8_t)br;
    }

    // mark done
    ldrReading = false;
    // keep previousMillisLDR as the timestamp of the last full cycle (already set)
  }
}

/**
 * Loop
 */
void loop() {
  mainLoop();
  currentMillisMainLoop = millis();

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  debounceSmartButton();
  if (!apFileRead) {
    apFileRead = true;
    String ap = bootstrapManager.readValueFromFile(AP_FILENAME, AP_PARAM);
    if (!ap.isEmpty() && ap != ERROR && ap.toInt() != 0) {
      setApState(0);
      LedManager::setColor(0, 0, 0);
    }
  }
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (relayState && !ledManager.stateOn) {
    Globals::turnOffRelay();
  }
  netManager.getUDPStream();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
#if defined(ESP8266)
  if (currentMillisMainLoop - prevMillisPing > 30000) {
    prevMillisPing = currentMillisMainLoop;
    // Hey gateway, GlowWorm is here
    if (!pingESP.ping()) {
      WiFi.reconnect();
    }
  }
#endif
#endif

#if defined(ESP8266)
  manageLdr();
#endif

  if ((builtInLedStatus || resetLedStatus) && wifiReconnectAttemp == 0 && mqttReconnectAttemp == 0) {
    builtInLedStatus = false;
    resetLedStatus = false;
    if (!ledManager.stateOn) {
      LedManager::setColorNoSolid(0, 0, 0);
    }
    disconnectionTime = currentMillisMainLoop;
    LedManager::manageBuiltInLed(0, 0, 0);
  }

  ledManager.updateTransition();
}
