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
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM);
  if (!baudRateFromStorage.isEmpty() && baudRateFromStorage != ERROR && baudRateFromStorage.toInt() != 0) {
    baudRateInUse = baudRateFromStorage.toInt();
  }
  int baudRateToUse = setBaudRateInUse(baudRateInUse);
  Serial.begin(baudRateToUse);
  while (!Serial); // wait for serial attach
#if defined(ESP32)
  if (!SPIFFS.begin(true)) {
    SPIFFS.format();
  }
#endif
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
    whiteTemp = whiteTempInUse = whiteTempToUse.toInt();
    ledManager.setTemperature(whiteTemp);
  }
  Serial.print(F("\nUsing White temp="));
  Serial.println(whiteTempToUse);

#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
  MAC = WiFi.macAddress();
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // LED number from configuration storage
  String topicToUse = bootstrapManager.readValueFromFile(TOPIC_FILENAME, MQTT_PARAM);
  if (topicToUse != "null" && !topicToUse.isEmpty() && topicToUse != ERROR && topicToUse != topicInUse) {
    topicInUse = topicToUse;
    executeMqttSwap(topicInUse);
  }
  Serial.print(F("\nMQTT topic in use="));
  Serial.println(topicInUse);

  // Bootsrap setup() with Wifi and MQTT functions
  bootstrapManager.bootstrapSetup(manageDisconnections, manageHardwareButton, callback);
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

  String colorModeFromStorage = bootstrapManager.readValueFromFile(ledManager.COLOR_MODE_FILENAME, ledManager.COLOR_MODE_PARAM);
  if (!colorModeFromStorage.isEmpty() && colorModeFromStorage != ERROR && colorModeFromStorage.toInt() != 0) {
    colorMode = colorModeFromStorage.toInt();
  } else {
    colorMode = 0;
  }

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
  UDP.begin(UDP_PORT);
  broadcastUDP.begin(UDP_BROADCAST_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);
  fpsData.reserve(200);
  prefsData.reserve(200);
  listenOnHttpGet();
#if defined(ESP8266)
  // Hey gateway, GlowWorm is here
  delay(DELAY_500);
  pingESP.ping(WiFi.gatewayIP());
#endif
#endif

}

/**
 * Set gpio received by the Firefly Luciferin software
 * @param gpio gpio to use
 */
void setGpio(int gpio) {

  Serial.println("CHANGING GPIO");
  gpioInUse = gpio;
#if defined(ESP8266)
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[GPIO_PARAM] = gpioInUse;
  bootstrapManager.writeToLittleFS(gpioDoc, GPIO_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument gpioDoc(1024);
  gpioDoc[GPIO_PARAM] = gpioInUse;
  bootstrapManager.writeToSPIFFS(gpioDoc, GPIO_FILENAME);
#endif
  delay(20);

}



int setBaudRateInUse(int baudRate) {

  baudRateInUse = baudRate;
  int baudRateToUse = 0;
  switch (baudRate) {
    case 1: baudRateToUse = 230400; break;
    case 2: baudRateToUse = 460800; break;
    case 4: baudRateToUse = 921600; break;
    case 5: baudRateToUse = 1000000; break;
    case 6: baudRateToUse = 1500000; break;
    case 7: baudRateToUse = 2000000; break;
    default: baudRateToUse = 500000; break;
  }
  return baudRateToUse;

}

/**
 * Set baudRate received by the Firefly Luciferin software
 * @param baudRate int
 */
void setBaudRate(int baudRate) {

  Serial.println(F("CHANGING BAUDRATE"));
  setBaudRateInUse(baudRate);
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
  bootstrapManager.writeToLittleFS(baudrateDoc, BAUDRATE_FILENAME);
#endif
#if defined(ESP32)
  bootstrapManager.writeToSPIFFS(baudrateDoc, BAUDRATE_FILENAME);
  SPIFFS.end();
#endif
  delay(20);

}

/**
 * Set white temp received by the Firefly Luciferin software
 * @param baudRate int
 */
void setWhiteTemp(int whiteTemp) {

  Serial.println(F("CHANGING WHITE TEMP"));
  whiteTempInUse = whiteTemp;
  ledManager.setTemperature(whiteTemp);
  DynamicJsonDocument whiteTempDoc(1024);
  whiteTempDoc[WHITE_TEMP_PARAM] = whiteTempInUse;
#if defined(ESP8266)
  bootstrapManager.writeToLittleFS(whiteTempDoc, WHITE_TEMP_FILENAME);
#endif
#if defined(ESP32)
  bootstrapManager.writeToSPIFFS(whiteTempDoc, BAUDRATE_FILENAME);
  SPIFFS.end();
#endif
  delay(20);

}

/**
 * Set numled received by the Firefly Luciferin software
 * @param numLedFromLuciferin int
 */
void setNumLed(int numLedFromLuciferin) {

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

#ifdef TARGET_GLOWWORMLUCIFERINFULL

/**
 * MANAGE WIFI AND MQTT DISCONNECTION
 */
void manageDisconnections() {

  ledManager.setColor(0, 0, 0);
  delay(500);

}

/**
 * MQTT SUBSCRIPTIONS
 */
void manageQueueSubscription() {

  bootstrapManager.subscribe(lightSetTopic.c_str());
  bootstrapManager.subscribe(streamTopic.c_str(), 0);
  bootstrapManager.subscribe(cmndReboot.c_str());
  bootstrapManager.subscribe(updateStateTopic.c_str());
  bootstrapManager.subscribe(unsubscribeTopic.c_str());
  bootstrapManager.subscribe(firmwareConfigTopic.c_str());

}

/**
 * List on HTTP GET
 */
void listenOnHttpGet() {

  server.on("/", []() {
      stopUDP();
      server.send(200, F("text/html"), settingsPage);
      startUDP();
  });
  server.on("/setsettings", []() {
      stopUDP();
      server.send(200, F("text/html"), setSettingsPage);
      startUDP();
  });
  server.on(prefsTopic.c_str(), []() {
      prefsData = F("{\"VERSION\":\"");
      prefsData += VERSION;
      prefsData += F("\",\"cp\":\"");
      prefsData += red; prefsData += F(",");
      prefsData += green; prefsData += F(",");
      prefsData += blue;
      prefsData += F("\",\"toggle\":\"");
      prefsData += stateOn;
      prefsData += F("\",\"effect\":\"");
      prefsData += effectParam;
      prefsData += F("\",\"whiteTemp\":\"");
      prefsData += whiteTemp;
      prefsData += F("\",\"wifi\":\"");
      prefsData += bootstrapManager.getWifiQuality();
      prefsData += F("\",\"framerate\":\"");
      prefsData += framerate;
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(GET_SETTINGS, []() {
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
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(("/" + lightSetTopic).c_str(), []() {
      httpCallback(processJson);
      JsonVariant requestedEffect = bootstrapManager.jsonDoc["effect"];
      if (mqttIP.length() > 0) {
        if (requestedEffect == "GlowWorm" || requestedEffect == "GlowWormWifi") {
          bootstrapManager.publish(lightStateTopic.c_str(), START_FF, true);
        } else {
          bootstrapManager.publish(lightStateTopic.c_str(), STOP_FF, true);
          framerate = framerateCounter = 0;
        }
      } else {
#if defined(ESP8266)
        if (remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
          if (!remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
          broadcastUDP.beginPacket(remoteBroadcastPort, UDP_BROADCAST_PORT);
          if (requestedEffect == "GlowWorm" || requestedEffect == "GlowWormWifi") {
            broadcastUDP.print(START_FF);
          } else {
            broadcastUDP.print(STOP_FF);
            framerate = framerateCounter = 0;
          }
          broadcastUDP.endPacket();
        }
      }
  });
  server.on(("/" + cmndReboot).c_str(), []() {
      httpCallback(processGlowWormLuciferinRebootCmnd);
  });
  server.on(("/" + updateStateTopic).c_str(), []() {
      httpCallback(processUpdate);
  });
  server.on(("/" + unsubscribeTopic).c_str(), []() {
      httpCallback(processUnSubscribeStream);
  });
  server.on(("/" + firmwareConfigTopic).c_str(), []() {
      httpCallback(processFirmwareConfig);
  });
  server.onNotFound([]() {
      server.send(404, F("text/plain"), ("Glow Worm Luciferin: Uri not found ") + server.uri());
  });
  server.on("/setsettings", []() {
      server.send(200, "text/html", setSettingsPage);
  });
  server.on("/setting", []() {
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
      ledManager.setColorMode(colorModeParam.toInt());
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
      setNumLed(lednum.toInt());
      delay(DELAY_200);
#elif defined(ESP32)
      SPIFFS.format();
        if (SPIFFS.begin()) {
            File configFile = SPIFFS.open("/setup.json", "w");
            if (!configFile) {
              Serial.println(F("Failed to open [setup.json] file for writing"));
            } else {
              serializeJsonPretty(doc, Serial);
              serializeJson(doc, configFile);
              configFile.close();
              Serial.println("[setup.json] written correctly");
            }
            Serial.println(F("Saving lednum"));
            setNumLed(lednum.toInt());
            delay(DELAY_200);
          } else {
            Serial.println(F("Failed to mount FS for write"));
          }
#endif
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
void stopUDP() {
  UDP.stop();
  servingWebPages = true;
  delay(10);
}

/*
 * Start UDP broadcast while serving pages
 */
void startUDP() {
  delay(10);
  servingWebPages = false;
  UDP.begin(UDP_PORT);
}

/**
 * MANAGE HARDWARE BUTTON
 */
void manageHardwareButton() {
  // no hardware button at the moment
}

/**
 * START CALLBACK
 * @param topic MQTT topic
 * @param payload MQTT payload
 * @param length MQTT message length
 */
void callback(char *topic, byte *payload, unsigned int length) {

  if (streamTopic.equals(topic)) {
    if (effect == Effect::GlowWormWifi) {
      if (JSON_STREAM) {
        jsonStream(payload, length);
      } else {
        fromMqttStreamToStrip(reinterpret_cast<char *>(payload));
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    bootstrapManager.parseQueueMsg(topic, payload, length);
    if (cmndReboot.equals(topic)) {
      processGlowWormLuciferinRebootCmnd();
    } else if (lightSetTopic.equals(topic)) {
      processJson();
    } else if (updateStateTopic.equals(topic)) {
      processMqttUpdate();
    } else if (firmwareConfigTopic.equals(topic)) {
      processFirmwareConfig();
    } else if (unsubscribeTopic.equals(topic)) {
      processUnSubscribeStream();
    }
    if (stateOn) {
      ledManager.setColor(map(red, 0, 255, 0, brightness), map(green, 0, 255, 0, brightness), map(blue, 0, 255, 0, brightness));
    } else {
      ledManager.setColor(0,0,0);
    }
  }

}

/**
 * Execute callback from HTTP payload
 * @param callback to execute using HTTP payload
 */
void httpCallback(bool (*callback)()) {

  bootstrapManager.jsonDoc.clear();
  String payload = server.arg(F("payload"));
  bootstrapManager.parseHttpMsg(payload, payload.length());
  callback();
  if (stateOn) {
    ledManager.setColor(map(red, 0, 255, 0, brightness), map(green, 0, 255, 0, brightness), map(blue, 0, 255, 0, brightness));
  } else {
    ledManager.setColor(0,0,0);
  }
  server.send(200, F("text/plain"), F("OK"));

}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void fromMqttStreamToStrip(char *payload) {

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
  if (brightness != audioBrightness) {
    brightness = audioBrightness;
  }
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != NULL) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
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
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]) {

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
  if (brightness != audioBrightness) {
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
      setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != NULL) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
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

/**
 * [DEPRECATED] Stream RGB in JSON format
 * NOT: JSON stream requires:
 *  - '-D MAX_JSON_OBJECT_SIZE=200'
 *  - '-D SMALL_JSON_OBJECT_SIZE=50'
 *  - '-D MQTT_MAX_PACKET_SIZE=2048'
 */
void jsonStream(byte *payload, unsigned int length) {

  bootstrapManager.jsonDocBigSize.clear();
  deserializeJson(bootstrapManager.jsonDocBigSize, (const byte*) payload, length);
  int numLedFromLuciferin = bootstrapManager.jsonDocBigSize[LED_NUM_PARAM];
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      setNumLed(numLedFromLuciferin);
    }
    // (leds, 0, (ledManager.dynamicLedNum) * sizeof(struct CRGB));
    JsonArray stream = bootstrapManager.jsonDocBigSize["stream"];
    if (ledManager.dynamicLedNum < FIRST_CHUNK) {
      for (uint16_t i = 0; i < ledManager.dynamicLedNum; i++) {
        int rgb = stream[i];
        leds[i].r = (rgb >> 16 & 0xFF);
        leds[i].g = (rgb >> 8 & 0xFF);
        leds[i].b = (rgb >> 0 & 0xFF);
      }
      FastLED.show();
    } else {
      if (ledManager.dynamicLedNum >= FIRST_CHUNK) {
        part = bootstrapManager.jsonDocBigSize["part"];
      }
      if (part == 1) {
        for (uint16_t i = 0; i < FIRST_CHUNK; i++) {
          int rgb = stream[i];
          leds[i].r = (rgb >> 16 & 0xFF);
          leds[i].g = (rgb >> 8 & 0xFF);
          leds[i].b = (rgb >> 0 & 0xFF);
        }
      } else if (part == 2) {
        int j = 0;
        for (uint16_t i = FIRST_CHUNK; i >= FIRST_CHUNK && i < SECOND_CHUNK; i++) {
          int rgb = stream[j];
          leds[i].r = (rgb >> 16 & 0xFF);
          leds[i].g = (rgb >> 8 & 0xFF);
          leds[i].b = (rgb >> 0 & 0xFF);
          j++;
        }
        if (ledManager.dynamicLedNum < SECOND_CHUNK) {
          FastLED.show();
        }
      } else if (part == 3) {
        int j = 0;
        for (uint16_t i = SECOND_CHUNK; i >= SECOND_CHUNK && i < THIRD_CHUNK; i++) {
          int rgb = stream[j];
          leds[i].r = (rgb >> 16 & 0xFF);
          leds[i].g = (rgb >> 8 & 0xFF);
          leds[i].b = (rgb >> 0 & 0xFF);
          j++;
        }
        if (ledManager.dynamicLedNum < THIRD_CHUNK) {
          FastLED.show();
        }
      } else if (part == 4) {
        int j = 0;
        for (int16_t i = THIRD_CHUNK; i >= THIRD_CHUNK && i < NUM_LEDS; i++) {
          int rgb = stream[j];
          leds[i].r = (rgb >> 16 & 0xFF);
          leds[i].g = (rgb >> 8 & 0xFF);
          leds[i].b = (rgb >> 0 & 0xFF);
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
 * Process Firmware Configuration sent from Firefly Luciferin
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processFirmwareConfig() {

  boolean espRestart = false;
  if (bootstrapManager.jsonDoc.containsKey("MAC")) {
    String macToUpdate = bootstrapManager.jsonDoc["MAC"];
    Serial.println(macToUpdate);
    if (macToUpdate == MAC) {
      // GPIO
      if (bootstrapManager.jsonDoc.containsKey(GPIO_PARAM)) {
        int gpio = (int) bootstrapManager.jsonDoc[GPIO_PARAM];
        if (gpio != 0 && gpioInUse != gpio) {
          setGpio(gpio);
          reinitLEDTriggered = true;
        }
      }
      // BAUDRATE
      if (bootstrapManager.jsonDoc.containsKey(BAUDRATE_PARAM)) {
        int baudrate = (int) bootstrapManager.jsonDoc[BAUDRATE_PARAM];
        if (baudrate != 0 && baudRateInUse != baudrate) {
          setBaudRate(baudrate);
          espRestart = true;
        }
      }
      // SWAP TOPIC
      boolean topicRestart = swapMqttTopic();
      if (topicRestart) espRestart = true;
      // Restart if needed
      if (reinitLEDTriggered) {
        reinitLEDTriggered = false;
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
bool processUnSubscribeStream() {

  if (bootstrapManager.jsonDoc.containsKey("instance")) {
    String instance = bootstrapManager.jsonDoc["instance"];
    String manager = bootstrapManager.jsonDoc["manager"];
    if (manager.equals(deviceName)) {
      bootstrapManager.unsubscribe(streamTopic.c_str());
      streamTopic = baseStreamTopic + instance;
      effect = Effect::GlowWormWifi;
      turnOnRelay();
      stateOn = true;
      bootstrapManager.subscribe(streamTopic.c_str(), 0);
    }
  }
  return true;

}

/**
 * Process JSON message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processJson() {

  lastLedUpdate = millis();

  if (bootstrapManager.jsonDoc.containsKey("state")) {
    String state = bootstrapManager.jsonDoc["state"];
    if (state == ON_CMD) {
      turnOnRelay();
      stateOn = true;
    } else if (state == OFF_CMD) {
      stateOn = false;
    }
  }

  if (bootstrapManager.jsonDoc.containsKey("color")) {
    red = bootstrapManager.jsonDoc["color"]["r"];
    green = bootstrapManager.jsonDoc["color"]["g"];
    blue = bootstrapManager.jsonDoc["color"]["b"];
    if (bootstrapManager.jsonDoc["color"].containsKey("colorMode")) {
      uint8_t newColorMode = bootstrapManager.jsonDoc["color"]["colorMode"];
      ledManager.setColorModeInit(newColorMode);
    }
  }

  if (bootstrapManager.jsonDoc.containsKey("brightness")) {
    brightness = bootstrapManager.jsonDoc["brightness"];
  }

  if (bootstrapManager.jsonDoc.containsKey("whitetemp")) {
    whiteTemp = bootstrapManager.jsonDoc["whitetemp"];
    if (whiteTemp != 0 && whiteTempInUse != whiteTemp) {
      setWhiteTemp(whiteTemp);
    }
  }

  if (bootstrapManager.jsonDoc.containsKey("effect")) {
    JsonVariant requestedEffect = bootstrapManager.jsonDoc["effect"];
    if (bootstrapManager.jsonDoc.containsKey("MAC")) {
      if (bootstrapManager.jsonDoc["MAC"] == MAC) {
        if (requestedEffect == "GlowWorm") {
          effect = Effect::GlowWorm;
          lastLedUpdate = millis();
        } else if (requestedEffect == "GlowWormWifi") {
          effect = Effect::GlowWormWifi;
          lastStream = millis();
        }
      }
    }
    else {
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
    }

  }

  return true;

}

/**
 * Send microcontroller state
 * For debug: ESP.getFreeHeap() ESP.getHeapFragmentation() ESP.getMaxFreeBlockSize()
 */
void sendStatus() {

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
      bootstrapManager.publish(fpsTopic.c_str(), fpsData.c_str(), false);
    } else {
#if defined(ESP8266)
      if (remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
        if (!remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
        broadcastUDP.beginPacket(remoteBroadcastPort, UDP_BROADCAST_PORT);
        broadcastUDP.print(fpsData.c_str());
        broadcastUDP.endPacket();
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    JsonObject root = bootstrapManager.jsonDoc.to<JsonObject>();
    JsonObject color = root.createNestedObject(F("color"));
    root[F("state")] = (stateOn) ? ON_CMD : OFF_CMD;
    color[F("r")] = red;
    color[F("g")] = green;
    color[F("b")] = blue;
    color[F("colorMode")] = colorMode;
    root[F("brightness")] = brightness;
    switch (effect) {
      case Effect::GlowWormWifi:
        effectParam = F("GlowWormWifi");
        break;
      case Effect::GlowWorm:
        effectParam = F("GlowWorm");
        break;
      case Effect::solid: effectParam = F("solid"); break;
      case Effect::bpm: effectParam = F("bpm"); break;
      case Effect::fire: effectParam = F("fire"); break;
      case Effect::twinkle: effectParam = F("twinkle"); break;
      case Effect::rainbow: effectParam = F("rainbow"); break;
      case Effect::chase_rainbow: effectParam = F("chase rainbow"); break;
      case Effect::solid_rainbow: effectParam = F("solid rainbow"); break;
      case Effect::mixed_rainbow: effectParam = F("mixed rainbow"); break;
    }
    root[F("effect")] = effectParam;
    root[F("deviceName")] = deviceName;
    root[F("IP")] = microcontrollerIP;
    root[F("wifi")] = bootstrapManager.getWifiQuality();
    root[F("MAC")] = MAC;
    root[F("ver")] = VERSION;
    root[F("framerate")] = framerate;
    root[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
    root[F("board")] = F("ESP8266");
#elif defined(ESP32)
    root["board"] = "ESP32";
#endif
    root[LED_NUM_PARAM] = String(ledManager.dynamicLedNum);
    root[F("gpio")] = gpioInUse;
    root[F("mqttopic")] = topicInUse;

    if (effect == Effect::solid && !stateOn) {
      ledManager.setColor(0, 0, 0);
    }

    // This topic should be retained, we don't want unknown values on battery voltage or wifi signal
    if (mqttIP.length() > 0) {
      bootstrapManager.publish(lightStateTopic.c_str(), root, true);
    } else {
      String output;
      serializeJson(root, output);
#if defined(ESP8266)
      if (remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
        if (!remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
        broadcastUDP.beginPacket(remoteBroadcastPort, UDP_BROADCAST_PORT);
        broadcastUDP.print(output.c_str());
        broadcastUDP.endPacket();
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
bool processMqttUpdate() {

  if (bootstrapManager.jsonDoc.containsKey(F("update"))) {
    return processUpdate();
  }
  return true;

}

/**
 * Handle web server for the upgrade process
 * @return true if message is correctly processed
 */
bool processUpdate() {

  Serial.println(F("Starting web server"));
  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      bool error = Update.hasError();
      server.send(200, "text/plain", error ? "KO" : "OK");
      if (!error) {
        if (mqttIP.length() > 0) {
          bootstrapManager.publish(updateResultStateTopic.c_str(), deviceName.c_str(), false);
        } else {
#if defined(ESP8266)
          if (remoteBroadcastPort.isSet()) {
#elif defined(ESP32)
            if (!remoteBroadcastPort.toString().equals(F("0.0.0.0"))) {
#endif
            broadcastUDP.beginPacket(remoteBroadcastPort, UDP_BROADCAST_PORT);
            broadcastUDP.print(deviceName.c_str());
            broadcastUDP.endPacket();
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
bool processGlowWormLuciferinRebootCmnd() {

  if (bootstrapManager.jsonDoc[VALUE] == OFF_CMD) {
    stateOn = false;
    sendStatus();
    delay(1500);
    ESP.restart();
  }
  return true;

}

/**
 * Swap MQTT topic with a custom one received from Firefly Luciferin
 * @param json StaticJsonDocument
 * @return true if mqtt has been swapper and need reboot
 */
bool swapMqttTopic() {

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
 * Execute the MQTT topic swap
 * @param customtopic new topic to use
 */
void executeMqttSwap(String customtopic) {

  Serial.println("Swapping topic=" + customtopic);
  topicInUse = customtopic;
  swapTopicUnsubscribe();
  swapTopicReplace(customtopic);
  swapTopicSubscribe();

}

/**
 * Unsubscribe from the default MQTT topic
 */
void swapTopicUnsubscribe() {

  bootstrapManager.unsubscribe(lightStateTopic.c_str());
  bootstrapManager.unsubscribe(updateStateTopic.c_str());
  bootstrapManager.unsubscribe(updateResultStateTopic.c_str());
  bootstrapManager.unsubscribe(lightSetTopic.c_str());
  bootstrapManager.unsubscribe(baseStreamTopic.c_str());
  bootstrapManager.unsubscribe(streamTopic.c_str());
  bootstrapManager.unsubscribe(unsubscribeTopic.c_str());

}

/**
 * Swap MQTT topi with the custom one
 * @param customtopic custom MQTT topic to use, received by Firefly Luciferin
 */
void swapTopicReplace(String customtopic) {

  lightStateTopic.replace(BASE_TOPIC, customtopic);
  updateStateTopic.replace(BASE_TOPIC, customtopic);
  updateResultStateTopic.replace(BASE_TOPIC, customtopic);
  lightSetTopic.replace(BASE_TOPIC, customtopic);
  baseStreamTopic.replace(BASE_TOPIC, customtopic);
  streamTopic.replace(BASE_TOPIC, customtopic);
  unsubscribeTopic.replace(BASE_TOPIC, customtopic);
  fpsTopic.replace(BASE_TOPIC, customtopic);

}

/**
 * Subscribe to custom MQTT topic
 */
void swapTopicSubscribe() {

  bootstrapManager.subscribe(lightStateTopic.c_str());
  bootstrapManager.subscribe(updateStateTopic.c_str());
  bootstrapManager.subscribe(updateResultStateTopic.c_str());
  bootstrapManager.subscribe(lightSetTopic.c_str());
  bootstrapManager.subscribe(baseStreamTopic.c_str());
  bootstrapManager.subscribe(streamTopic.c_str(), 0);
  bootstrapManager.subscribe(unsubscribeTopic.c_str());

}

#endif







/**
 * Check connection and turn off the LED strip if no input received
 */
void checkConnection() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);
  server.handleClient();

  EVERY_N_SECONDS(10) {
    // No updates since 7 seconds, turn off LEDs
    if ((!breakLoop && (effect == Effect::GlowWorm) && (millis() > lastLedUpdate + 10000)) ||
        (!breakLoop && (effect == Effect::GlowWormWifi) && (millis() > lastStream + 10000))) {
      breakLoop = true;
      effect = Effect::solid;
      stateOn = false;
      turnOffRelay();
    }
    framerate = framerateCounter > 0 ? framerateCounter / 10 : 0;
    framerateCounter = 0;
    sendStatus();
  }
#elif  TARGET_GLOWWORMLUCIFERINLIGHT
  EVERY_N_SECONDS(15) {
    // No updates since 15 seconds, turn off LEDs
    if(millis() > lastLedUpdate + 10000){
      ledManager.setColor(0, 0, 0);
      turnOffRelay();
    }
  }
#endif
  sendSerialInfo();

}

int serialRead() {

  return !breakLoop ? Serial.read() : -1;

}

void mainLoop() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  checkConnection();
#endif

  // GLOW_WORM_LUCIFERIN, serial connection with Firefly Luciferin
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (effect == Effect::GlowWorm) {
#endif
    if (!led_state) led_state = true;

    for (i = 0; i < sizeof prefix; ++i) {
      waitLoop:
      while (!breakLoop && !Serial.available()) checkConnection();
      if (breakLoop || prefix[i] == serialRead()) continue;
      i = 0;
      goto waitLoop;
    }

    while (!breakLoop && !Serial.available()) checkConnection();
    hi = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    lo = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    loSecondPart = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    usbBrightness = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    gpio = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    baudRate = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    whiteTemp = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    fireflyEffect = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    fireflyColorMode = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    chk = serialRead();

    if (!breakLoop && (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ whiteTemp ^ fireflyEffect ^ fireflyColorMode ^ 0x55))) {
      i = 0;
      goto waitLoop;
    }

#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    if (!relayState) {
      turnOnRelay();
    }
#endif

    if (usbBrightness != brightness) {
      brightness = usbBrightness;
    }

    if (gpio != 0 && gpioInUse != gpio && (gpio == 2 || gpio == 3 || gpio == 5 || gpio == 16)) {
      setGpio(gpio);
      reinitLEDTriggered = true;
    }

    int numLedFromLuciferin = lo + loSecondPart + 1;
    if (ledManager.dynamicLedNum != numLedFromLuciferin && numLedFromLuciferin < NUM_LEDS) {
      setNumLed(numLedFromLuciferin);
      reinitLEDTriggered = true;
    }

    if (reinitLEDTriggered) {
      reinitLEDTriggered = false;
      ledManager.initLeds();
      breakLoop = true;
    }

    if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 7)) {
      setBaudRate(baudRate);
      ESP.restart();
    }

    if (whiteTemp != 0 && whiteTempInUse != whiteTemp) {
      whiteTempInUse = whiteTemp;
      setWhiteTemp(whiteTemp);
    }

    // If MQTT is enabled but using USB cable, effect is 0 and is set via MQTT callback
    if (fireflyEffect != 0 && fireflyEffectInUse != fireflyEffect) {
      fireflyEffectInUse = fireflyEffect;
      switch (fireflyEffectInUse) {
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
        case 100: fireflyEffectInUse = 0; break;
      }
    }

    ledManager.setColorModeInit(fireflyColorMode);

    // memset(leds, 0, (numLedFromLuciferin) * sizeof(struct CRGB));
    // Serial.readBytes( (char*)leds, numLedFromLuciferin * 3);
    for (uint16_t i = 0; i < (numLedFromLuciferin); i++) {
      byte r, g, b;
      while (!breakLoop && !Serial.available()) checkConnection();
      r = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      g = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      b = serialRead();
      if (fireflyEffectInUse <= 5) {
        ledManager.setPixelColor(i, r, g, b);
      }
    }
    lastLedUpdate = millis();
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
    effectsManager.bpm(leds, currentPalette, targetPalette);
  }

  //EFFECT RAINBOW
  if (effect == Effect::rainbow) {
    effectsManager.rainbow(leds, ledManager.dynamicLedNum);
  }

  //SOLID RAINBOW
  if (effect == Effect::solid_rainbow) {
    effectsManager.solidRainbow(leds, ledManager.dynamicLedNum);
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
    effectsManager.mixedRainbow(checkConnection, leds, ledManager.dynamicLedNum);
  }

}

/**
 * Pinned on CORE1 on ESP32, max performance with Serial
 */
void loop() {

  mainLoop();

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (relayState && !stateOn) {
    turnOffRelay();
  }
  getUDPStream();
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

/**
 * Parse UDP packet
 */
void getUDPStream() {

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
 * Turn ON the relay
 */
void turnOnRelay() {

  if (!relayState) {
    relayState = true;
#if defined(ESP8266)
    digitalWrite(RELAY_PIN, HIGH);
#elif defined(ESP32)
    digitalWrite(RELAY_PIN_DIG, HIGH);
    digitalWrite(RELAY_PIN_PICO, HIGH);
#endif
    delay(100);
  }

}

/**
 * Turn OFF the relay
 */
void turnOffRelay() {

  if (relayState) {
    relayState = false;
    delay(100);
#if defined(ESP8266)
    digitalWrite(RELAY_PIN, LOW);
#elif defined(ESP32)
    digitalWrite(RELAY_PIN_DIG, LOW);
    digitalWrite(RELAY_PIN_PICO, LOW);
#endif
  }

}

/**
 * Send serial info
 */
void sendSerialInfo() {

  EVERY_N_SECONDS(10) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    framerate = framerateCounter > 0 ? framerateCounter / 10 : 0;
    framerateCounter = 0;
    Serial.printf("framerate:%s\n", (String((framerate > 0.5 ? framerate : 0),1)).c_str());
    Serial.printf("firmware:%s\n", "LIGHT");
#else
    Serial.printf("firmware:%s\n", "FULL");
    Serial.printf("mqttopic:%s\n", topicInUse.c_str());
#endif
    Serial.printf("ver:%s\n", VERSION);
    Serial.printf("lednum:%d\n", ledManager.dynamicLedNum);
#if defined(ESP32)
    Serial.printf("board:%s\n", "ESP32");
#elif defined(ESP8266)
    Serial.printf("board:%s\n", "ESP8266");
#endif
    Serial.printf("MAC:%s\n", MAC.c_str());
    Serial.printf("gpio:%d\n", gpioInUse);
    Serial.printf("baudrate:%d\n", baudRateInUse);
    Serial.printf("effect:%d\n", effect);
    Serial.printf("colorMode:%d\n", colorMode);

  }

}