/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "GlowWormLuciferin.h"

/**
 * Setup function
 */
void setup() {

  // if fastDisconnectionManagement we need to execute the disconnection callback immediately
  fastDisconnectionManagement = true;
  // BaudRate from configuration storage
#if defined(ESP32)
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM, false);
#else
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM);
#endif
  if (!baudRateFromStorage.isEmpty() && baudRateFromStorage != ERROR && baudRateFromStorage.toInt() != 0) {
    baudRateInUse = baudRateFromStorage.toInt();
  }
  int baudRateToUse = globals.setBaudRateInUse(baudRateInUse);
  Serial.begin(baudRateToUse);
  while (!Serial); // wait for serial attach
  Serial.print(F("BAUDRATE IN USE="));
  Serial.println(baudRateToUse);

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
    whiteTemp = ledManager.whiteTempInUse = whiteTempToUse.toInt();
  }
  Serial.print(F("\nUsing White temp="));
  Serial.println(whiteTempToUse);

#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
  MAC = WiFi.macAddress();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // LED number from configuration storage
  String topicToUse = bootstrapManager.readValueFromFile(networkManager.TOPIC_FILENAME, networkManager.MQTT_PARAM);
  if (topicToUse != "null" && !topicToUse.isEmpty() && topicToUse != ERROR && topicToUse != networkManager.topicInUse) {
    networkManager.topicInUse = topicToUse;
    networkManager.executeMqttSwap(networkManager.topicInUse);
  }
  Serial.print(F("\nMQTT topic in use="));
  Serial.println(networkManager.topicInUse);

  // Bootsrap setup() with Wifi and MQTT functions
  bootstrapManager.bootstrapSetup(NetworkManager::manageDisconnections, NetworkManager::manageHardwareButton, NetworkManager::callback);
#endif

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
    case 5: gpioInUse = 5; break;
    case 3: gpioInUse = 3; break;
    case 16: gpioInUse = 16; break;
    default: gpioInUse = 2; break;
  }
  Serial.print(F("GPIO IN USE="));
  Serial.println(gpioInUse);


  // Color mode from configuration storage
  String colorModeFromStorage = bootstrapManager.readValueFromFile(ledManager.COLOR_MODE_FILENAME, ledManager.COLOR_MODE_PARAM);
  if (!colorModeFromStorage.isEmpty() && colorModeFromStorage != ERROR && colorModeFromStorage.toInt() != 0) {
    colorMode = colorModeFromStorage.toInt();
  }
  Serial.print(F("COLOR_MODE IN USE="));
  Serial.println(colorMode);
  ledManager.initLeds();

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
  networkManager.fpsData.reserve(200);
  networkManager.prefsData.reserve(200);
  networkManager.listenOnHttpGet();
#if defined(ESP8266)
  // Hey gateway, GlowWorm is here
  delay(DELAY_500);
  pingESP.ping(WiFi.gatewayIP());
#endif
#endif

}

/**
 * Read serial or break the reading
 * @return -1 if loop must break
 */
int serialRead() {

  return !breakLoop ? Serial.read() : -1;

}

/**
 * Main loop
 */
void mainLoop() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  networkManager.checkConnection();
#endif

  // GLOW_WORM_LUCIFERIN, serial connection with Firefly Luciferin
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (effect == Effect::GlowWorm) {
#endif
    if (!ledManager.led_state) ledManager.led_state = true;

    for (i = 0; i < prefixLength; ++i) {
      waitLoop:
      while (!breakLoop && !Serial.available()) networkManager.checkConnection();
      if (breakLoop || prefix[i] == serialRead()) continue;
      i = 0;
      goto waitLoop;
    }

    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    hi = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    lo = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    loSecondPart = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    usbBrightness = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    gpio = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    baudRate = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    whiteTemp = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    fireflyEffect = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    fireflyColorMode = serialRead();
    while (!breakLoop && !Serial.available()) networkManager.checkConnection();
    chk = serialRead();

    if (!breakLoop && (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ whiteTemp ^ fireflyEffect ^ fireflyColorMode ^ 0x55))) {
      i = 0;
      goto waitLoop;
    }

#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    if (!relayState) {
      globals.turnOnRelay();
    }
#endif

    if (usbBrightness != brightness) {
      brightness = usbBrightness;
    }

    if (gpio != 0 && gpioInUse != gpio && (gpio == 2 || gpio == 3 || gpio == 5 || gpio == 16)) {
      globals.setGpio(gpio);
      ledManager.reinitLEDTriggered = true;
    }

    int numLedFromLuciferin = lo + loSecondPart + 1;
    if (ledManager.dynamicLedNum != numLedFromLuciferin && numLedFromLuciferin < NUM_LEDS) {
      ledManager.setNumLed(numLedFromLuciferin);
      ledManager.reinitLEDTriggered = true;
    }

    if (ledManager.reinitLEDTriggered) {
      ledManager.reinitLEDTriggered = false;
      ledManager.initLeds();
      breakLoop = true;
    }

    if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 7)) {
      globals.setBaudRate(baudRate);
      ESP.restart();
    }

    if (whiteTemp != 0 && ledManager.whiteTempInUse != whiteTemp) {
      ledManager.whiteTempInUse = whiteTemp;
      ledManager.setWhiteTemp(whiteTemp);
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
        effect = Effect::GlowWorm; break;
#endif
        case 5: effect = Effect::solid; break;
        case 6: effect = Effect::fire; break;
        case 7: effect = Effect::twinkle; break;
        case 8: effect = Effect::bpm; break;
        case 9: effect = Effect::mixed_rainbow; break;
        case 10: effect = Effect::rainbow; break;
        case 11: effect = Effect::chase_rainbow; break;
        case 12: effect = Effect::solid_rainbow; break;
        case 100: ledManager.fireflyEffectInUse = 0; break;
      }
    }

    ledManager.setColorModeInit(fireflyColorMode);

    // memset(leds, 0, (numLedFromLuciferin) * sizeof(struct CRGB));
    // Serial.readBytes( (char*)leds, numLedFromLuciferin * 3);
    for (uint16_t i = 0; i < (numLedFromLuciferin); i++) {
      byte r, g, b;
      while (!breakLoop && !Serial.available()) networkManager.checkConnection();
      r = serialRead();
      while (!breakLoop && !Serial.available()) networkManager.checkConnection();
      g = serialRead();
      while (!breakLoop && !Serial.available()) networkManager.checkConnection();
      b = serialRead();
      if (ledManager.fireflyEffectInUse <= 5) {
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
    effectsManager.fire(55, 120, 15, ledManager.dynamicLedNum);
  }

  //TWINKLE
  if (effect == Effect::twinkle) {
    effectsManager.twinkleRandom(20, 100, false, ledManager.dynamicLedNum);
  }

  //CHASE RAINBOW
  if (effect == Effect::chase_rainbow) {
    effectsManager.theaterChaseRainbow(ledManager.dynamicLedNum);
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
  if (relayState && !ledManager.stateOn) {
    globals.turnOffRelay();
  }
  networkManager.getUDPStream();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
#if defined(ESP8266)
  EVERY_N_SECONDS(30) {
    // Hey gateway, GlowWorm is here
    bool res = pingESP.ping(WiFi.gatewayIP());
    if (!res) {
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

}
