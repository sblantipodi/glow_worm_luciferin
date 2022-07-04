/*
  NetworkManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "NetworkManager.h"

uint16_t NetworkManager::part = 1;
boolean NetworkManager::firmwareUpgrade = false;
size_t NetworkManager::updateSize = 0;
String NetworkManager::fpsData;

/**
 * Parse UDP packet
 */
void NetworkManager::getUDPStream() {

  if (!servingWebPages) {
    // If packet received...
    uint16_t packetSize = UDP.parsePacket();
    UDP.read(packet, UDP_MAX_BUFFER_SIZE);
    if (effect == Effect::GlowWormWifi) {
      if (packetSize > 20) {
        packet[packetSize] = '\0';
        fromUDPStreamToStrip(packet);
      }
    }
    // If packet received...
    uint16_t packetSizeBroadcast = broadcastUDP.parsePacket();
    broadcastUDP.read(packetBroadcast, UDP_MAX_BUFFER_SIZE);
    if (packetSizeBroadcast == 4) {
      remoteBroadcastPort = broadcastUDP.remoteIP();
    }
  }

}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void NetworkManager::fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]) {

  uint32_t myLeds;
  char delimiters[] = ",";
  char *ptr;
  char *saveptr;
  char *ptrAtoi;

  uint16_t index;
  ptr = strtok_r(payload, delimiters, &saveptr);
  // Discard packet if header does not match the correct one
  if (strcmp(ptr, "DPsoftware") != 0) {
    return;
  }
  ptr = strtok_r(NULL, delimiters, &saveptr);
  uint16_t numLedFromLuciferin = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  uint8_t audioBrightness = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  if (brightness != audioBrightness && !ldrEnabled) {
    brightness = audioBrightness;
  }
  uint8_t chunkTot, chunkNum;
  chunkTot = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  chunkNum = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  index = UDP_CHUNK_SIZE * chunkNum;
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      ledManager.setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != NULL) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      if (!ldrContinuous && ldrEnabled && ldrReading) {
        ledManager.setPixelColor(index, 0, 0, 0);
      } else {
        ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
      }
      index++;
      ptr = strtok_r(NULL, delimiters, &saveptr);
    }
  }
  if (effect != Effect::solid) {
    if (chunkNum == chunkTot - 1) {
      framerateCounter++;
      lastStream = millis();
      ledManager.ledShow();
    }
  }

}

#ifdef TARGET_GLOWWORMLUCIFERINFULL

/**
 * MANAGE WIFI AND MQTT DISCONNECTION
 */
void NetworkManager::manageDisconnections() {

  ledManager.setColor(0, 0, 0);
  delay(500);

}

/**
 * MQTT SUBSCRIPTIONS
 */
void NetworkManager::manageQueueSubscription() {

  bootstrapManager.subscribe(networkManager.lightSetTopic.c_str());
  bootstrapManager.subscribe(networkManager.streamTopic.c_str(), 0);
  bootstrapManager.subscribe(networkManager.cmndReboot.c_str());
  bootstrapManager.subscribe(networkManager.updateStateTopic.c_str());
  bootstrapManager.subscribe(networkManager.unsubscribeTopic.c_str());
  bootstrapManager.subscribe(networkManager.firmwareConfigTopic.c_str());

}

/**
 * List on HTTP GET
 */
void NetworkManager::listenOnHttpGet() {

  server.on("/", [this]() {
      stopUDP();
      server.send(200, F("text/html"), settingsPage);
      startUDP();
  });
  server.on("/setsettings", [this]() {
      stopUDP();
      server.send(200, F("text/html"), setSettingsPage);
      startUDP();
  });
  server.on(networkManager.prefsTopic.c_str(), [this]() {
      prefsData = F("{\"VERSION\":\"");
      prefsData += VERSION;
      prefsData += F("\",\"cp\":\"");
      prefsData += ledManager.red; prefsData += F(",");
      prefsData += ledManager.green; prefsData += F(",");
      prefsData += ledManager.blue;
      prefsData += F("\",\"toggle\":\"");
      prefsData += ledManager.stateOn;
      prefsData += F("\",\"effect\":\"");
      prefsData += globals.effectToString(effect);
      prefsData += F("\",\"whiteTemp\":\"");
      prefsData += whiteTempInUse;
      prefsData += F("\",\"brightness\":\"");
      prefsData += brightness;
      prefsData += F("\",\"wifi\":\"");
      prefsData += bootstrapManager.getWifiQuality();
      prefsData += F("\",\"framerate\":\"");
      prefsData += framerate;
      if (ldrEnabled) {
        prefsData += F("\",\"ldr\":\"");
        prefsData += ((ldrValue * 100) / LDR_DIVIDER);
      }
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(networkManager.GET_SETTINGS, [this]() {
      prefsData = F("{\"deviceName\":\"");
      prefsData += deviceName;
      prefsData += F("\",\"dhcp\":\"");
      prefsData += dhcpInUse;
      prefsData += F("\",\"ip\":\"");
      prefsData += microcontrollerIP;
      prefsData += F("\",\"mqttuser\":\"");
      prefsData += mqttuser;
      prefsData += F("\",\"mqttIp\":\"");
      prefsData += mqttIP;
      prefsData += F("\",\"mqttpass\":\"");
      prefsData += mqttpass;
      prefsData += F("\",\"mqttPort\":\"");
      prefsData += mqttPort;
      prefsData += F("\",\"lednum\":\"");
      prefsData += ledManager.dynamicLedNum;
      prefsData += F("\",\"gpio\":\"");
      prefsData += gpioInUse;
      prefsData += F("\",\"colorMode\":\"");
      prefsData += colorMode;
      prefsData += F("\",\"br\":\"");
      prefsData += baudRateInUse;
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(networkManager.GET_LDR, [this]() {
      prefsData = F("{\"ldrEnabled\":\"");
      prefsData += ldrEnabled;
      prefsData += F("\",\"ldrContinuous\":\"");
      prefsData += ldrContinuous;
      prefsData += F("\",\"ldrMin\":\"");
      prefsData += ldrMin;
      prefsData += F("\",\"ldrMax\":\"");
      prefsData += ldrMax;
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(("/" + networkManager.lightSetTopic).c_str(), [this]() {
      httpCallback(processJson);
      JsonVariant requestedEffect = bootstrapManager.jsonDoc["effect"];
      if (mqttIP.length() > 0) {
        if (requestedEffect == "GlowWorm" || requestedEffect == "GlowWormWifi") {
          bootstrapManager.publish(networkManager.lightStateTopic.c_str(), START_FF, true);
        } else {
          bootstrapManager.publish(networkManager.lightStateTopic.c_str(), STOP_FF, true);
          framerate = framerateCounter = 0;
        }
      } else {
#if defined(ESP8266)
        if (networkManager.remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
          if (!networkManager.remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
          networkManager.broadcastUDP.beginPacket(networkManager.remoteBroadcastPort, UDP_BROADCAST_PORT);
          if (requestedEffect == "GlowWorm" || requestedEffect == "GlowWormWifi") {
            networkManager.broadcastUDP.print(START_FF);
          } else {
            networkManager.broadcastUDP.print(STOP_FF);
            framerate = framerateCounter = 0;
          }
          networkManager.broadcastUDP.endPacket();
        }
      }
  });
  server.on(("/" + networkManager.cmndReboot).c_str(), [this]() {
      httpCallback(processGlowWormLuciferinRebootCmnd);
  });
  server.on(("/" + networkManager.updateStateTopic).c_str(), [this]() {
      httpCallback(processUpdate);
  });
  server.on(("/" + networkManager.unsubscribeTopic).c_str(), [this]() {
      httpCallback(processUnSubscribeStream);
  });
  server.on(("/" + networkManager.firmwareConfigTopic).c_str(), [this]() {
      httpCallback(processFirmwareConfig);
  });
  server.onNotFound([]() {
      server.send(404, F("text/plain"), ("Glow Worm Luciferin: Uri not found ") + server.uri());
  });
  server.on("/setsettings", []() {
      server.send(200, "text/html", setSettingsPage);
  });
  server.on("/setldr", []() {
      server.send(200, "text/html", setLdrPage);
  });
  server.on("/ldr", [this]() {
      stopUDP();
      String ldrEnabled = server.arg("ldrEnabled");
      String ldrContinuous = server.arg("ldrContinuous");
      String ldrMin = server.arg("ldrMin");
      String ldrMax = server.arg("ldrMax");
      DynamicJsonDocument doc(1024);
      Serial.println("ldrEnabled");
      Serial.println(ldrEnabled);
      Serial.println("ldrContinuous");
      Serial.println(ldrContinuous);
      Serial.println("ldrMin");
      Serial.println(ldrMin);
      Serial.println("ldrMax");
      Serial.println(ldrMax);
      doc["ldrEnabled"] = ldrEnabled;
      doc["ldrContinuous"] = ldrContinuous;
      doc["ldrMin"] = ldrMin;
      doc["ldrMax"] = ldrMax;
      content = F("Success: rebooting the microcontroller using your credentials.");
      statusCode = 200;
      delay(DELAY_500);
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "text/plain", content);
      delay(DELAY_500);
      ledManager.setLdr(ldrEnabled == "true", ldrContinuous == "true", ldrMin, ldrMax);
      delay(DELAY_1000);
#if defined(ESP8266) || defined(ESP32)
      ESP.restart();
#endif
  });

  server.on("/setting", [this]() {
      stopUDP();
      String deviceName = server.arg("deviceName");
      String microcontrollerIP = server.arg("microcontrollerIP");
      String mqttCheckbox = server.arg("mqttCheckbox");
      String mqttIP = server.arg("mqttIP");
      String mqttPort = server.arg("mqttPort");
      String mqttuser = server.arg("mqttuser");
      String mqttpass = server.arg("mqttpass");
      String additionalParam = server.arg("additionalParam");
      String colorModeParam = server.arg("colorMode");
      String lednum = server.arg("lednum");
      String br = server.arg("br");
      DynamicJsonDocument doc(1024);
      if (deviceName.length() > 0 && ((mqttCheckbox == "false") || (mqttIP.length() > 0 && mqttPort.length() > 0))) {
        Serial.println("deviceName");
        Serial.println(deviceName);
        Serial.println("microcontrollerIP");
        if (microcontrollerIP.length() == 0) {
          microcontrollerIP = "DHCP";
        }
        Serial.println(microcontrollerIP);
        Serial.println("qsid");
        Serial.println(qsid);
        Serial.println("qpass");
        Serial.println(qpass);
        Serial.println("OTApass");
        Serial.println(OTApass);
        Serial.println("mqttIP");
        Serial.println(mqttIP);
        Serial.println("mqttPort");
        Serial.println(mqttPort);
        Serial.println("mqttuser");
        Serial.println(mqttuser);
        Serial.println("mqttpass");
        Serial.println(mqttpass);
        Serial.println("additionalParam");
        Serial.println(additionalParam);
        Serial.println("lednum");
        Serial.println(lednum);
        Serial.println("colorMode");
        Serial.println(colorModeParam);
        Serial.println("br");
        Serial.println(br);
        doc["deviceName"] = deviceName;
        doc["microcontrollerIP"] = microcontrollerIP;
        doc["qsid"] = qsid;
        doc["qpass"] = qpass;
        doc["OTApass"] = OTApass;
        if (mqttCheckbox.equals("true")) {
          doc["mqttIP"] = mqttIP;
          doc["mqttPort"] = mqttPort;
          doc["mqttuser"] = mqttuser;
          doc["mqttpass"] = mqttpass;
        } else {
          doc["mqttIP"] = "";
          doc["mqttPort"] = "";
          doc["mqttuser"] = "";
          doc["mqttpass"] = "";
        }
        doc["additionalParam"] = additionalParam;
        content = F("Success: rebooting the microcontroller using your credentials.");
        statusCode = 200;
      } else {
        content = F("Error: missing required fields.");
        statusCode = 404;
        Serial.println(F("Sending 404"));
      }
      delay(DELAY_500);
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "text/plain", content);
      delay(DELAY_500);
#if defined(ESP8266)
      // Write to LittleFS
      Serial.println(F("Saving setup.json"));
      File jsonFile = LittleFS.open("/setup.json", "w");
      if (!jsonFile) {
        Serial.println(F("Failed to open [setup.json] file for writing"));
      } else {
        serializeJsonPretty(doc, Serial);
        serializeJson(doc, jsonFile);
        jsonFile.close();
        Serial.println(F("[setup.json] written correctly"));
      }
      delay(DELAY_200);
      Serial.println(F("Saving lednum"));
      ledManager.setNumLed(lednum.toInt());
      delay(DELAY_200);
      Serial.println(F("Saving gpio"));
      globals.setGpio(additionalParam.toInt());
      delay(DELAY_200);
#elif defined(ESP32)
      if (SPIFFS.begin(false)) {
        File configFile = SPIFFS.open("/setup.json", "w");
        if (!configFile) {
          Serial.println(F("Failed to open [setup.json] file for writing"));
        } else {
          serializeJsonPretty(doc, Serial);
          serializeJson(doc, configFile);
          configFile.close();
          Serial.println("[setup.json] written correctly");
        }
        delay(DELAY_200);
        Serial.println(F("Saving lednum"));
        ledManager.setNumLed(lednum.toInt());
        delay(DELAY_200);
        Serial.println(F("Saving gpio"));
        globals.setGpio(additionalParam.toInt());
        delay(DELAY_200);
      } else {
        Serial.println(F("Failed to mount FS for write"));
      }
#endif
      delay(DELAY_500);
      ledManager.setColorMode(colorModeParam.toInt());
      delay(DELAY_500);
      globals.setBaudRateInUse(br.toInt());
      globals.setBaudRate(baudRateInUse);
      delay(DELAY_1000);
#if defined(ESP8266) || defined(ESP32)
      ESP.restart();
#endif
  });

  server.begin();

}

/**
 * Stop UDP broadcast while serving pages
 */
void NetworkManager::stopUDP() {

  networkManager.UDP.stop();
  networkManager.servingWebPages = true;
  delay(10);

}

/*
 * Start UDP broadcast while serving pages
 */
void NetworkManager::startUDP() {

  delay(10);
  networkManager.servingWebPages = false;
  networkManager.UDP.begin(UDP_PORT);

}

/**
 * MANAGE HARDWARE BUTTON
 */
void NetworkManager::manageHardwareButton() {
  // no hardware button at the moment
}

/**
 * START CALLBACK
 * @param topic MQTT topic
 * @param payload MQTT payload
 * @param length MQTT message length
 */
void NetworkManager::callback(char *topic, byte *payload, unsigned int length) {

  if (networkManager.streamTopic.equals(topic)) {
    if (effect == Effect::GlowWormWifi) {
      if (networkManager.JSON_STREAM) {
        jsonStream(payload, length);
      } else {
        fromMqttStreamToStrip(reinterpret_cast<char *>(payload));
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    bootstrapManager.parseQueueMsg(topic, payload, length);
    if (networkManager.cmndReboot.equals(topic)) {
      processGlowWormLuciferinRebootCmnd();
    } else if (networkManager.lightSetTopic.equals(topic)) {
      processJson();
    } else if (networkManager.updateStateTopic.equals(topic)) {
      processMqttUpdate();
    } else if (networkManager.firmwareConfigTopic.equals(topic)) {
      processFirmwareConfig();
    } else if (networkManager.unsubscribeTopic.equals(topic)) {
      processUnSubscribeStream();
    }
    if (ledManager.stateOn) {
      ledManager.setColor(map(ledManager.red, 0, 255, 0, brightness), map(ledManager.green, 0, 255, 0, brightness),
                          map(ledManager.blue, 0, 255, 0, brightness));
    } else {
      ledManager.setColor(0,0,0);
    }
  }

}

/**
 * Execute callback from HTTP payload
 * @param callback to execute using HTTP payload
 */
void NetworkManager::httpCallback(bool (*callback)()) {

  bootstrapManager.jsonDoc.clear();
  String payload = server.arg(F("payload"));
  bootstrapManager.parseHttpMsg(payload, payload.length());
  callback();
  if (ledManager.stateOn) {
    ledManager.setColor(map(ledManager.red, 0, 255, 0, brightness), map(ledManager.green, 0, 255, 0, brightness),
                        map(ledManager.blue, 0, 255, 0, brightness));
  } else {
    ledManager.setColor(0,0,0);
  }
  server.send(200, F("text/plain"), F("OK"));

}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void NetworkManager::fromMqttStreamToStrip(char *payload) {

  uint32_t myLeds;
  char delimiters[] = ",";
  char *ptr;
  char *saveptr;
  char *ptrAtoi;

  uint16_t index = 0;
  ptr = strtok_r(payload, delimiters, &saveptr);
  uint16_t numLedFromLuciferin = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  uint8_t audioBrightness = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(NULL, delimiters, &saveptr);
  if (brightness != audioBrightness && !ldrEnabled) {
    brightness = audioBrightness;
  }
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      ledManager.setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != NULL) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      if (!ldrContinuous && ldrEnabled && ldrReading) {
        ledManager.setPixelColor(index, 0, 0, 0);
      } else {
        ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
      }
      index++;
      ptr = strtok_r(NULL, delimiters, &saveptr);
    }
  }
  if (effect != Effect::solid) {
    framerateCounter++;
    lastStream = millis();
    ledManager.ledShow();
  }

}

/**
 * [DEPRECATED] Stream RGB in JSON format
 * NOT: JSON stream requires:
 *  - '-D MAX_JSON_OBJECT_SIZE=200'
 *  - '-D SMALL_JSON_OBJECT_SIZE=50'
 *  - '-D MQTT_MAX_PACKET_SIZE=2048'
 */
void NetworkManager::jsonStream(byte *payload, unsigned int length) {

  bootstrapManager.jsonDocBigSize.clear();
  deserializeJson(bootstrapManager.jsonDocBigSize, (const byte*) payload, length);
  int numLedFromLuciferin = bootstrapManager.jsonDocBigSize[LED_NUM_PARAM];
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      ledManager.setNumLed(numLedFromLuciferin);
    }
    // (ledManager.leds, 0, (ledManager.dynamicLedNum) * sizeof(struct CRGB));
    JsonArray stream = bootstrapManager.jsonDocBigSize["stream"];
    if (ledManager.dynamicLedNum < FIRST_CHUNK) {
      for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
        int rgb = stream[i];
        ledManager.leds[i].r = (rgb >> 16 & 0xFF);
        ledManager.leds[i].g = (rgb >> 8 & 0xFF);
        ledManager.leds[i].b = (rgb >> 0 & 0xFF);
      }
      FastLED.show();
    } else {
      if (ledManager.dynamicLedNum >= FIRST_CHUNK) {
        part = bootstrapManager.jsonDocBigSize["part"];
      }
      if (part == 1) {
        for (uint16_t i = 0; i < FIRST_CHUNK; i++) {
          int rgb = stream[i];
          ledManager.leds[i].r = (rgb >> 16 & 0xFF);
          ledManager.leds[i].g = (rgb >> 8 & 0xFF);
          ledManager.leds[i].b = (rgb >> 0 & 0xFF);
        }
      } else if (part == 2) {
        int j = 0;
        for (uint16_t i = FIRST_CHUNK; i >= FIRST_CHUNK && i < SECOND_CHUNK; i++) {
          int rgb = stream[j];
          ledManager.leds[i].r = (rgb >> 16 & 0xFF);
          ledManager.leds[i].g = (rgb >> 8 & 0xFF);
          ledManager.leds[i].b = (rgb >> 0 & 0xFF);
          j++;
        }
        if (ledManager.dynamicLedNum < SECOND_CHUNK) {
          FastLED.show();
        }
      } else if (part == 3) {
        int j = 0;
        for (uint16_t i = SECOND_CHUNK; i >= SECOND_CHUNK && i < THIRD_CHUNK; i++) {
          int rgb = stream[j];
          ledManager.leds[i].r = (rgb >> 16 & 0xFF);
          ledManager.leds[i].g = (rgb >> 8 & 0xFF);
          ledManager.leds[i].b = (rgb >> 0 & 0xFF);
          j++;
        }
        if (ledManager.dynamicLedNum < THIRD_CHUNK) {
          FastLED.show();
        }
      } else if (part == 4) {
        int j = 0;
        for (int16_t i = THIRD_CHUNK; i >= THIRD_CHUNK && i < NUM_LEDS; i++) {
          int rgb = stream[j];
          ledManager.leds[i].r = (rgb >> 16 & 0xFF);
          ledManager.leds[i].g = (rgb >> 8 & 0xFF);
          ledManager.leds[i].b = (rgb >> 0 & 0xFF);
          j++;
        }
        FastLED.show();
      }
    }
#ifdef TARGET_GLOWWORMLUCIFERINFULL
    if ((ledManager.dynamicLedNum < FIRST_CHUNK) || (ledManager.dynamicLedNum < SECOND_CHUNK && part == 2)
        || (ledManager.dynamicLedNum < THIRD_CHUNK && part == 3) || (part == 4)) {
      framerateCounter++;
    }
#endif
    lastStream = millis();
  }

}

/**
 * Swap MQTT topic with a custom one received from Firefly Luciferin
 * @param json StaticJsonDocument
 * @return true if mqtt has been swapper and need reboot
 */
boolean NetworkManager::swapMqttTopic() {

  boolean reboot = false;
  if (bootstrapManager.jsonDoc.containsKey(MQTT_PARAM)) {
    String customtopic = bootstrapManager.jsonDoc[MQTT_PARAM];
    if (customtopic != topicInUse) {
      // Write to storage
      Serial.println("SWAPPING MQTT_TOPIC");
      topicInUse = customtopic;
      DynamicJsonDocument topicDoc(1024);
      topicDoc[MQTT_PARAM] = topicInUse;
#if defined(ESP8266)
      bootstrapManager.writeToLittleFS(topicDoc, TOPIC_FILENAME);
#endif
#if defined(ESP32)
      bootstrapManager.writeToSPIFFS(topicDoc, TOPIC_FILENAME);
      SPIFFS.end();
#endif
      delay(20);
      executeMqttSwap(customtopic);
      reboot = true;
    }
  }
  return reboot;

}

/**
 * Process Firmware Configuration sent from Firefly Luciferin
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetworkManager::processFirmwareConfig() {

  boolean espRestart = false;
  if (bootstrapManager.jsonDoc.containsKey("MAC")) {
    String macToUpdate = bootstrapManager.jsonDoc["MAC"];
    Serial.println(macToUpdate);
    if (macToUpdate == MAC) {
      // GPIO
      if (bootstrapManager.jsonDoc.containsKey(GPIO_PARAM)) {
        int gpio = (int) bootstrapManager.jsonDoc[GPIO_PARAM];
        if (gpio != 0 && gpioInUse != gpio) {
          globals.setGpio(gpio);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // BAUDRATE
      if (bootstrapManager.jsonDoc.containsKey(BAUDRATE_PARAM)) {
        int baudrate = (int) bootstrapManager.jsonDoc[BAUDRATE_PARAM];
        if (baudrate != 0 && baudRateInUse != baudrate) {
          globals.setBaudRate(baudrate);
          espRestart = true;
        }
      }
      // COLOR_MODE
      if (bootstrapManager.jsonDoc.containsKey(ledManager.COLOR_MODE_PARAM)) {
        int colorModeParam = (int) bootstrapManager.jsonDoc[ledManager.COLOR_MODE_PARAM];
        if (colorMode != colorModeParam) {
          colorMode = colorModeParam;
          ledManager.setColorMode(colorMode);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // SWAP TOPIC
      boolean topicRestart = networkManager.swapMqttTopic();
      if (topicRestart) espRestart = true;
      // Restart if needed
      if (ledManager.reinitLEDTriggered) {
        ledManager.reinitLEDTriggered = false;
        ledManager.initLeds();
      }
      if (espRestart) {
        ESP.restart();
      }
    }
  }
  return true;

}

/**
 * Unsubscribe from the stream topic, we will use a specific topic for this instance
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetworkManager::processUnSubscribeStream() {

  if (bootstrapManager.jsonDoc.containsKey("instance")) {
    String instance = bootstrapManager.jsonDoc["instance"];
    String manager = bootstrapManager.jsonDoc["manager"];
    if (manager.equals(deviceName)) {
      bootstrapManager.unsubscribe(networkManager.streamTopic.c_str());
      networkManager.streamTopic = networkManager.baseStreamTopic + instance;
      effect = Effect::GlowWormWifi;
      globals.turnOnRelay();
      ledManager.stateOn = true;
      bootstrapManager.subscribe(networkManager.streamTopic.c_str(), 0);
    }
  }
  return true;

}

/**
 * Process JSON message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetworkManager::processJson() {

  ledManager.lastLedUpdate = millis();
  boolean skipMacCheck = ((bootstrapManager.jsonDoc.containsKey("MAC") && bootstrapManager.jsonDoc["MAC"] == MAC)
          || bootstrapManager.jsonDoc.containsKey("allInstances"));
  if (!bootstrapManager.jsonDoc.containsKey("MAC") || skipMacCheck) {
    if (bootstrapManager.jsonDoc.containsKey("state")) {
      String state = bootstrapManager.jsonDoc["state"];
      if (state == ON_CMD) {
        globals.turnOnRelay();
        ledManager.stateOn = true;
      } else if (state == OFF_CMD) {
        ledManager.stateOn = false;
      }
    }
    if (bootstrapManager.jsonDoc.containsKey("color")) {
      ledManager.red = bootstrapManager.jsonDoc["color"]["r"];
      ledManager.green = bootstrapManager.jsonDoc["color"]["g"];
      ledManager.blue = bootstrapManager.jsonDoc["color"]["b"];
    }
    if (bootstrapManager.jsonDoc.containsKey("brightness")) {
      brightness = bootstrapManager.jsonDoc["brightness"];
    }
    if (skipMacCheck) {
      if (bootstrapManager.jsonDoc.containsKey("whitetemp")) {
        uint8_t wt = bootstrapManager.jsonDoc["whitetemp"];
        if (wt != 0 && whiteTempInUse != wt) {
          ledManager.setWhiteTemp(wt);
        }
      }
    }
    if (bootstrapManager.jsonDoc.containsKey("effect")) {
      JsonVariant requestedEffect = bootstrapManager.jsonDoc["effect"];
      if (requestedEffect == "bpm") effect = Effect::bpm;
      else if (requestedEffect == "fire") effect = Effect::fire;
      else if (requestedEffect == "twinkle") effect = Effect::twinkle;
      else if (requestedEffect == "rainbow") effect = Effect::rainbow;
      else if (requestedEffect == "chase rainbow") effect = Effect::chase_rainbow;
      else if (requestedEffect == "solid rainbow") effect = Effect::solid_rainbow;
      else if (requestedEffect == "mixed rainbow") effect = Effect::mixed_rainbow;
      else {
        effect = Effect::solid;
        breakLoop = true;
      }
      if (skipMacCheck) {
        if (requestedEffect == "GlowWorm") {
          effect = Effect::GlowWorm;
          ledManager.lastLedUpdate = millis();
        } else if (requestedEffect == "GlowWormWifi") {
          effect = Effect::GlowWormWifi;
          lastStream = millis();
        }
      }
    }
  }
  return true;

}

/**
 * Send microcontroller state
 * For debug: ESP.getFreeHeap() ESP.getHeapFragmentation() ESP.getMaxFreeBlockSize()
 */
void NetworkManager::sendStatus() {

  // Skip JSON framework for lighter processing during the stream
  if (effect == Effect::GlowWorm || effect == Effect::GlowWormWifi) {
    fpsData = F("{\"deviceName\":\"");
    fpsData += deviceName;
    fpsData += F("\",\"MAC\":\"");
    fpsData += MAC;
    fpsData += F("\",\"lednum\":\"");
    fpsData += ledManager.dynamicLedNum;
    fpsData += F("\",\"framerate\":\"");
    fpsData += framerate;
    fpsData += F("\",\"wifi\":\"");
    fpsData += bootstrapManager.getWifiQuality();
    fpsData += F("\"}");
    if (mqttIP.length() > 0) {
      bootstrapManager.publish(networkManager.fpsTopic.c_str(), fpsData.c_str(), false);
    } else {
#if defined(ESP8266)
      if (networkManager.remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
        if (!networkManager.remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
        networkManager.broadcastUDP.beginPacket(networkManager.remoteBroadcastPort, UDP_BROADCAST_PORT);
        networkManager.broadcastUDP.print(fpsData.c_str());
        networkManager.broadcastUDP.endPacket();
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    JsonObject root = bootstrapManager.jsonDoc.to<JsonObject>();
    JsonObject color = root.createNestedObject(F("color"));
    root[F("state")] = (ledManager.stateOn) ? ON_CMD : OFF_CMD;
    color[F("r")] = ledManager.red;
    color[F("g")] = ledManager.green;
    color[F("b")] = ledManager.blue;
    color[F("colorMode")] = colorMode;
    root[F("brightness")] = brightness;
    root[F("effect")] = globals.effectToString(effect);
    root[F("deviceName")] = deviceName;
    root[F("IP")] = microcontrollerIP;
    root[F("wifi")] = bootstrapManager.getWifiQuality();
    root[F("MAC")] = MAC;
    root[F("ver")] = VERSION;
    root[F("framerate")] = framerate;
    root[F("ldr")] = ldrValue;
    root[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
    root[F("board")] = F("ESP8266");
#elif defined(ESP32)
    root["board"] = "ESP32";
#endif
    root[LED_NUM_PARAM] = String(ledManager.dynamicLedNum);
    root[F("gpio")] = gpioInUse;
    root[F("mqttopic")] = networkManager.topicInUse;
    root[F("whitetemp")] = whiteTempInUse;

    if (effect == Effect::solid && !ledManager.stateOn) {
      ledManager.setColor(0, 0, 0);
    }

    // This topic should be retained, we don't want unknown values on battery voltage or wifi signal
    if (mqttIP.length() > 0) {
      bootstrapManager.publish(networkManager.lightStateTopic.c_str(), root, true);
    } else {
      String output;
      serializeJson(root, output);
#if defined(ESP8266)
      if (networkManager.remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
        if (!networkManager.remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
        networkManager.broadcastUDP.beginPacket(networkManager.remoteBroadcastPort, UDP_BROADCAST_PORT);
        networkManager.broadcastUDP.print(output.c_str());
        networkManager.broadcastUDP.endPacket();
      }
    }
  }
  // Built in led triggered
  ledTriggered = true;

}

/**
* Handle web server for the upgrade process
* @return true if message is correctly processed
*/
bool NetworkManager::processMqttUpdate() {

  if (bootstrapManager.jsonDoc.containsKey(F("update"))) {
    return processUpdate();
  }
  return true;

}

/**
 * Handle web server for the upgrade process
 * @return true if message is correctly processed
 */
bool NetworkManager::processUpdate() {

  Serial.println(F("Starting web server"));
  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      bool error = Update.hasError();
      server.send(200, "text/plain", error ? "KO" : "OK");
      if (!error) {
        if (mqttIP.length() > 0) {
          bootstrapManager.publish(networkManager.updateResultStateTopic.c_str(), deviceName.c_str(), false);
        } else {
#if defined(ESP8266)
          if (networkManager.remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
            if (!networkManager.remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
            networkManager.broadcastUDP.beginPacket(networkManager.remoteBroadcastPort, UDP_BROADCAST_PORT);
            networkManager.broadcastUDP.print(deviceName.c_str());
            networkManager.broadcastUDP.endPacket();
          }
        }
      }
      delay(DELAY_500);
      ESP.restart();
  }, []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
#if defined(ESP32)
        updateSize = UPDATE_SIZE_UNKNOWN;
#elif defined(ESP8266)
        updateSize = 480000;
#endif
        if (!Update.begin(updateSize)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
  });
  server.begin();
  Serial.println(F("Web server started"));
  firmwareUpgrade = true;

  return true;

}

/**
 * Reboot the microcontroller
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetworkManager::processGlowWormLuciferinRebootCmnd() {

  if (bootstrapManager.jsonDoc[VALUE] == OFF_CMD) {
    ledManager.stateOn = false;
    sendStatus();
    delay(1500);
    ESP.restart();
  }
  return true;

}



/**
 * Execute the MQTT topic swap
 * @param customtopic new topic to use
 */
void NetworkManager::executeMqttSwap(String customtopic) {

  Serial.println("Swapping topic=" + customtopic);
  networkManager.topicInUse = customtopic;
  swapTopicUnsubscribe();
  swapTopicReplace(customtopic);
  swapTopicSubscribe();

}

/**
 * Unsubscribe from the default MQTT topic
 */
void NetworkManager::swapTopicUnsubscribe() {

  bootstrapManager.unsubscribe(networkManager.lightStateTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.updateStateTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.updateResultStateTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.lightSetTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.baseStreamTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.streamTopic.c_str());
  bootstrapManager.unsubscribe(networkManager.unsubscribeTopic.c_str());

}

/**
 * Swap MQTT topi with the custom one
 * @param customtopic custom MQTT topic to use, received by Firefly Luciferin
 */
void NetworkManager::swapTopicReplace(String customtopic) {

  networkManager.lightStateTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.updateStateTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.updateResultStateTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.lightSetTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.baseStreamTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.streamTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.unsubscribeTopic.replace(networkManager.BASE_TOPIC, customtopic);
  networkManager.fpsTopic.replace(networkManager.BASE_TOPIC, customtopic);

}

/**
 * Subscribe to custom MQTT topic
 */
void NetworkManager::swapTopicSubscribe() {

  bootstrapManager.subscribe(networkManager.lightStateTopic.c_str());
  bootstrapManager.subscribe(networkManager.updateStateTopic.c_str());
  bootstrapManager.subscribe(networkManager.updateResultStateTopic.c_str());
  bootstrapManager.subscribe(networkManager.lightSetTopic.c_str());
  bootstrapManager.subscribe(networkManager.baseStreamTopic.c_str());
  bootstrapManager.subscribe(networkManager.streamTopic.c_str(), 0);
  bootstrapManager.subscribe(networkManager.unsubscribeTopic.c_str());

}

#endif

/**
 * Check connection and turn off the LED strip if no input received
 */
void NetworkManager::checkConnection() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);
  server.handleClient();

  EVERY_N_SECONDS(10) {
    // No updates since 7 seconds, turn off LEDs
    if ((!breakLoop && (effect == Effect::GlowWorm) && (millis() > ledManager.lastLedUpdate + 10000)) ||
        (!breakLoop && (effect == Effect::GlowWormWifi) && (millis() > lastStream + 10000))) {
      breakLoop = true;
      effect = Effect::solid;
      ledManager.stateOn = false;
      globals.turnOffRelay();
    }
    framerate = framerateCounter > 0 ? framerateCounter / 10 : 0;
    framerateCounter = 0;
    NetworkManager::sendStatus();
  }
#elif  TARGET_GLOWWORMLUCIFERINLIGHT
  EVERY_N_SECONDS(15) {
    // No updates since 15 seconds, turn off LEDs
    if(millis() > ledManager.lastLedUpdate + 10000){
      ledManager.setColor(0, 0, 0);
      globals.turnOffRelay();
    }
  }
#endif
  globals.sendSerialInfo();

}