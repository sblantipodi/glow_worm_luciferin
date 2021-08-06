/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright (C) 2020 - 2021  Davide Perini

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

  * Components:
   - Arduino C++ sketch running on an ESP8266EX D1 Mini from Lolin running @ 160MHz
   - WS2812B 5V LED Strip
   - 3.3V/5V Logic Level Converter
   - 220Ω resistor
   - 1000uf capacitor for 5V power stabilization
   - Raspberry + Home Assistant for Web GUI, automations and MQTT server (HA is optional but an MQTT server is needed)
   - Google Home Mini for Voice Recognition (optional)
  NOTE: 3.3V to 5V logic level converter is not mandatory but it is really recommended, without it,
  some input on the led strip digital pin could be lost. If you use a 5V microcontroller like Arduino Nano or similar you don't need it.
*/

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "GlowWormLuciferin.h"

/**
 * Dynamic PIN Template
 */
#if defined(ESP32)
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1800KbpsMethod>* ledsESP32 = NULL; // Hardware, ALL GPIO, yes serial read/write
#else
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>* ledsDMA = NULL; // Hardware DMA, GPIO3, no serial read, yes serial write
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod>* ledsUART = NULL; // Hardware UART, GPIO2, yes serial read/write
NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBangWs2812xMethod>* ledsStandard = NULL; // No hardware, ALL GPIO, yes serial read/write
#endif

/**
 * Setup function
 */
void setup() {

#if defined(ESP32)
  if (!SPIFFS.begin()) {
    SPIFFS.format();
  }
#endif

  // BaudRate from configuration storage
  String baudRateFromStorage = bootstrapManager.readValueFromFile(BAUDRATE_FILENAME, BAUDRATE_PARAM);
  if (!baudRateFromStorage.isEmpty() && baudRateFromStorage != ERROR && baudRateFromStorage.toInt() != 0) {
    baudRateInUse = baudRateFromStorage.toInt();
  }
  int baudRateToUse = setBaudRateInUse(baudRateInUse);
  Serial.begin(baudRateToUse);
  Serial.print(F("BAUDRATE IN USE="));
  Serial.println(baudRateToUse);

  // LED number from configuration storage
  String ledNumToUse = bootstrapManager.readValueFromFile(LED_NUM_FILENAME, LED_NUM_PARAM);
  if (!ledNumToUse.isEmpty() && ledNumToUse != ERROR && ledNumToUse.toInt() != 0) {
    dynamicLedNum = ledNumToUse.toInt();
  }
  Serial.print("\nUsing LEDs=");
  Serial.println(dynamicLedNum);

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
    gpioToUse = additionalParam.toInt();
  }
  Serial.print(F("SAVED GPIO="));
  Serial.println(gpioToUse);
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

  initLeds();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

#if defined(ESP32)
  xTaskCreatePinnedToCore(
          tcpTask, /* Task function. */
          "tcpTask", /* name of task. */
          32192, /* Stack size of task */
          NULL, /* parameter of the task */
          2, /* priority of the task */
          &handleTcpTask, /* Task handle to keep track of created task */
          0);
  xTaskCreatePinnedToCore(
          serialTask, /* Task function. */
          "serialTask", /* name of task. */
          32192, /* Stack size of task */
          NULL, /* parameter of the task */
          2, /* priority of the task */
          &handleSerialTask, /* Task handle to keep track of created task */
          1);
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Begin listening to UDP port
  UDP.begin(UDP_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);
#endif

}


/**
 * Set gpio received by the Firefly Luciferin software
 * @param gpio itn
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

  Serial.println("CHANGING BAUDRATE");
  setBaudRateInUse(baudRate);
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
  bootstrapManager.writeToLittleFS(baudrateDoc, BAUDRATE_FILENAME);
#endif
#if defined(ESP32)
  bootstrapManager.writeToSPIFFS(baudrateDoc, BAUDRATE_FILENAME);
#endif
  delay(20);

}

/**
 * Set numled received by the Firefly Luciferin software
 * @param numLedFromLuciferin int
 */
void setNumLed(int numLedFromLuciferin) {

  dynamicLedNum = numLedFromLuciferin;
#if defined(ESP8266)
  DynamicJsonDocument numLedDoc(1024);
  numLedDoc[LED_NUM_PARAM] = dynamicLedNum;
  bootstrapManager.writeToLittleFS(numLedDoc, LED_NUM_FILENAME);
#endif
#if defined(ESP32)
  DynamicJsonDocument numLedDoc(1024);
  numLedDoc[LED_NUM_PARAM] = dynamicLedNum;
  bootstrapManager.writeToSPIFFS(numLedDoc, LED_NUM_FILENAME);
#endif
  delay(20);

}

#ifdef TARGET_GLOWWORMLUCIFERINFULL

/**
 * MANAGE WIFI AND MQTT DISCONNECTION
 */
void manageDisconnections() {

  setColor(0, 0, 0);

}

/**
 * MQTT SUBSCRIPTIONS
 */
void manageQueueSubscription() {

  bootstrapManager.subscribe(helper.string2char(lightSetTopic));
  bootstrapManager.subscribe(helper.string2char(streamTopic), 0);
  bootstrapManager.subscribe(CMND_AMBI_REBOOT);
  bootstrapManager.subscribe(helper.string2char(updateStateTopic));
  bootstrapManager.subscribe(helper.string2char(unsubscribeTopic));
  bootstrapManager.subscribe(helper.string2char(firmwareConfigTopic));

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
        fromStreamToStrip(reinterpret_cast<char *>(payload), false);
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    bootstrapManager.parseQueueMsg(topic, payload, length);
    if (strcmp(topic, CMND_AMBI_REBOOT) == 0) {
      processGlowWormLuciferinRebootCmnd();
    } else if (lightSetTopic.equals(topic)) {
      processJson();
    } else if (updateStateTopic.equals(topic)) {
      processUpdate();
    } else if (firmwareConfigTopic.equals(topic)) {
      processFirmwareConfig();
    } else if (unsubscribeTopic.equals(topic)) {
      processUnSubscribeStream();
    }
    if (stateOn) {
      realRed = map(red, 0, 255, 0, brightness);
      realGreen = map(green, 0, 255, 0, brightness);
      realBlue = map(blue, 0, 255, 0, brightness);
    } else {
      realRed = 0;
      realGreen = 0;
      realBlue = 0;
    }
    startFade = true;
    inFade = false; // Kill the current fade

  }

}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 * @param isUdpStream UDP stream or MQTT stream
 */
void fromStreamToStrip(char *payload, boolean isUdpStream) {

  int myLeds;
  char delimiters[] = ",";
  char *ptr;
  int index = 0;
  ptr = strtok(payload, delimiters);
  int numLedFromLuciferin = atoi(ptr);
  ptr = strtok(NULL, delimiters);
  int audioBrightness = atoi(ptr);
  ptr = strtok(NULL, delimiters);
  if (brightness != audioBrightness) {
    brightness = audioBrightness;
  }
  int chunkTot, chunkNum;
  if (isUdpStream) {
    chunkTot = atoi(ptr);
    ptr = strtok(NULL, delimiters);
    chunkNum = atoi(ptr);
    ptr = strtok(NULL, delimiters);
    index = UDP_CHUNK_SIZE * chunkNum;
    //Serial.println(chunkTot);
    //Serial.println(chunkNum);
  }
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (dynamicLedNum != numLedFromLuciferin) {
      setNumLed(numLedFromLuciferin);
      initLeds();
    }
    while (ptr != NULL) {
      myLeds = atoi(ptr);
      setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
      index++;
      ptr = strtok(NULL, delimiters);
    }
  }
  if (effect != Effect::solid) {
    if (isUdpStream) {
      if (chunkNum == chunkTot - 1) {
        framerateCounter++;
        lastStream = millis();
        ledShow();
      }
    } else {
      framerateCounter++;
      lastStream = millis();
      ledShow();
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
    if (dynamicLedNum != numLedFromLuciferin) {
      setNumLed(numLedFromLuciferin);
    }
    // (leds, 0, (dynamicLedNum) * sizeof(struct CRGB));
    JsonArray stream = bootstrapManager.jsonDocBigSize["stream"];
    if (dynamicLedNum < FIRST_CHUNK) {
      for (uint16_t i = 0; i < dynamicLedNum; i++) {
        int rgb = stream[i];
        leds[i].r = (rgb >> 16 & 0xFF);
        leds[i].g = (rgb >> 8 & 0xFF);
        leds[i].b = (rgb >> 0 & 0xFF);
      }
      FastLED.show();
    } else {
      if (dynamicLedNum >= FIRST_CHUNK) {
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
        if (dynamicLedNum < SECOND_CHUNK) {
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
        if (dynamicLedNum < THIRD_CHUNK) {
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
    if ((dynamicLedNum < FIRST_CHUNK) || (dynamicLedNum < SECOND_CHUNK && part == 2)
        || (dynamicLedNum < THIRD_CHUNK && part == 3) || (part == 4)) {
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
        initLeds();
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
      bootstrapManager.unsubscribe(helper.string2char(streamTopic));
      streamTopic = baseStreamTopic + instance;
      effect = Effect::GlowWormWifi;
      turnOnRelay();
      stateOn = true;
      bootstrapManager.subscribe(helper.string2char(streamTopic), 0);
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
      onbeforeflash = false;
    }
  }

  flash = false;
  if (stateOn) {   //if the light is turned on and the light isn't flashing
    onbeforeflash = true;
  }

  if (bootstrapManager.jsonDoc.containsKey("color")) {
    red = bootstrapManager.jsonDoc["color"]["r"];
    green = bootstrapManager.jsonDoc["color"]["g"];
    blue = bootstrapManager.jsonDoc["color"]["b"];
  }

  if (bootstrapManager.jsonDoc.containsKey("brightness")) {
    brightness = bootstrapManager.jsonDoc["brightness"];
  }

  if (bootstrapManager.jsonDoc.containsKey("whitetemp")) {
    int whitetemp = bootstrapManager.jsonDoc["whitetemp"];
    setTemperature(whitetemp);
  }

  if (bootstrapManager.jsonDoc.containsKey("transition")) {
    transitionTime = bootstrapManager.jsonDoc["transition"];
  } else if (effect == Effect::solid) {
    transitionTime = 0;
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
      else if (requestedEffect == "rainbow") effect = Effect::rainbow;
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
 */
void sendStatus() {
  // Skip JSON framework for lighter processing during the stream
  if (effect == Effect::GlowWorm || effect == Effect::GlowWormWifi) {
    bootstrapManager.publish(helper.string2char(fpsTopic), helper.string2char("{\"deviceName\":\""+deviceName+"\",\"MAC\":\""+MAC+"\",\"lednum\":\""+dynamicLedNum+"\",\"framerate\":\""+framerate+"\"}"), false);
  } else {
    bootstrapManager.jsonDoc.clear();
    JsonObject root = bootstrapManager.jsonDoc.to<JsonObject>();
    JsonObject color = root.createNestedObject("color");
    root["state"] = (stateOn) ? ON_CMD : OFF_CMD;
    color["r"] = red;
    color["g"] = green;
    color["b"] = blue;
    root["brightness"] = brightness;
    switch (effect) {
      case Effect::GlowWormWifi:
        root["effect"] = "GlowWormWifi";
        break;
      case Effect::GlowWorm:
        root["effect"] = "GlowWorm";
        break;
      case Effect::solid: root["effect"] = "solid"; break;
      case Effect::bpm: root["effect"] = "bpm"; break;
      case Effect::rainbow: root["effect"] = "rainbow"; break;
      case Effect::solid_rainbow: root["effect"] = "solid rainbow"; break;
      case Effect::mixed_rainbow: root["effect"] = "mixed rainbow"; break;
    }
    root["deviceName"] = deviceName;
    root["IP"] = microcontrollerIP;
    root["MAC"] = MAC;
    root["ver"] = VERSION;
    root["framerate"] = framerate;
    root[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
    root["board"] = "ESP8266";
#elif defined(ESP32)
    root["board"] = "ESP32";
#endif
    root[LED_NUM_PARAM] = String(dynamicLedNum);
    root["gpio"] = gpioInUse;
    root["mqttopic"] = topicInUse;

    if (effect == Effect::solid && !stateOn) {
      setColor(0, 0, 0);
    }

    // This topic should be retained, we don't want unknown values on battery voltage or wifi signal
    bootstrapManager.publish(helper.string2char(lightStateTopic), root, true);
  }

#if defined(ESP32)
  vTaskDelay(1);
  //Serial.print("Task is running on: ");
  //Serial.println(xPortGetCoreID());
#endif

  // Built in led triggered
  ledTriggered = true;

}

/**
 * Handle web server for the upgrade process
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processUpdate() {

  if (bootstrapManager.jsonDoc.containsKey(F("update"))) {

    Serial.println(F("Starting web server"));
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        bool error = Update.hasError();
        server.send(200, "text/plain", error ? "KO" : "OK");
        if (!error) {
          bootstrapManager.publish(helper.string2char(updateResultStateTopic), helper.string2char(deviceName), false);
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

  }
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

  bootstrapManager.unsubscribe(helper.string2char(lightStateTopic));
  bootstrapManager.unsubscribe(helper.string2char(updateStateTopic));
  bootstrapManager.unsubscribe(helper.string2char(updateResultStateTopic));
  bootstrapManager.unsubscribe(helper.string2char(lightSetTopic));
  bootstrapManager.unsubscribe(helper.string2char(baseStreamTopic));
  bootstrapManager.unsubscribe(helper.string2char(streamTopic));
  bootstrapManager.unsubscribe(helper.string2char(unsubscribeTopic));

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

  bootstrapManager.subscribe(helper.string2char(lightStateTopic));
  bootstrapManager.subscribe(helper.string2char(updateStateTopic));
  bootstrapManager.subscribe(helper.string2char(updateResultStateTopic));
  bootstrapManager.subscribe(helper.string2char(lightSetTopic));
  bootstrapManager.subscribe(helper.string2char(baseStreamTopic));
  bootstrapManager.subscribe(helper.string2char(streamTopic), 0);
  bootstrapManager.subscribe(helper.string2char(unsubscribeTopic));

}

#endif

/**
 * Set White Temperature for Color Correction
 * @param whitetemp kelvin
 */
void setTemperature(int whitetemp) {

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

/**
 * Set led strip color
 * @param inR red color
 * @param inG green color
 * @param inB blu color
 */
void setColor(int inR, int inG, int inB) {

  for (int i = 0; i < NUM_LEDS; i++) {
    setPixelColor(i, inR, inG, inB);
  }
  ledShow();
  Serial.print(F("Setting LEDs: "));
  Serial.print(F("r: "));
  Serial.print(inR);
  Serial.print(F(", g: "));
  Serial.print(inG);
  Serial.print(F(", b: "));
  Serial.println(inB);

}

/**
 * Check connection and turn off the LED strip if no input received
 */
void checkConnection() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);

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
      setColor(0, 0, 0);
      turnOffRelay();
    }
  }
#endif
#if defined(ESP8266)
  sendSerialInfo();
#endif

}

int serialRead() {

#ifdef TARGET_GLOWWORMLUCIFERINFULL
#if defined(ESP32)
  delayMicroseconds(10);
#endif
#endif
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
    off_timer = millis();

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
    chk = serialRead();

    if (!breakLoop && (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ whiteTemp ^ fireflyEffect ^ 0x55))) {
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
    if (dynamicLedNum != numLedFromLuciferin && numLedFromLuciferin < NUM_LEDS) {
      setNumLed(numLedFromLuciferin);
      reinitLEDTriggered = true;
    }

    if (reinitLEDTriggered) {
      reinitLEDTriggered = false;
      initLeds();
    }

    if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 7)) {
      setBaudRate(baudRate);
      ESP.restart();
    }

    if (whiteTemp != 0 && whiteTempInUse != whiteTemp) {
      whiteTempInUse = whiteTemp;
      setTemperature(whiteTempInUse);
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
        case 6: effect = Effect::bpm; break;
        case 7: effect = Effect::mixed_rainbow; break;
        case 8: effect = Effect::rainbow; break;
        case 9: effect = Effect::solid_rainbow; break;
        case 100: fireflyEffectInUse = 0; break;
      }
    }

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
        setPixelColor(i, r, g, b);
      }
    }
    lastLedUpdate = millis();
    framerateCounter++;
    ledShow();

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
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
      setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
    }
    showleds();
  }

  //EFFECT RAINBOW
  if (effect == Effect::rainbow) {
    // FastLED's built-in rainbow generator
    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
    for (int i = 0; i < NUM_LEDS; i++) {
      setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
    }
    showleds();
  }

  //SOLID RAINBOW
  if (effect == Effect::solid_rainbow) {
    // FastLED's built-in rainbow generator
    fill_solid(leds, NUM_LEDS, CHSV(thishue, 255, 255));
    for (int i = 0; i < NUM_LEDS; i++) {
      setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
    }
    if (millis()-lastAnimSolidRainbow >= 90) {
      lastAnimSolidRainbow = millis();
      thishue++;
    }
    showleds();
  }

  //MIXED RAINBOW
  if (effect == Effect::mixed_rainbow) {
#ifdef TARGET_GLOWWORMLUCIFERINFULL
    if (millis()-lastAnim >= 10) {
      lastAnim = millis();
      mixedRainboxIndex++;
    }
#elif TARGET_GLOWWORMLUCIFERINLIGHT
    mixedRainboxIndex++;
#endif
    if(mixedRainboxIndex < 256) {
      for(int i = 0; i < dynamicLedNum; i++) {
        leds[i] = Scroll((i * 256 / dynamicLedNum + mixedRainboxIndex) % 256);
        setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
#ifdef TARGET_GLOWWORMLUCIFERINFULL
        checkConnection();
#endif
      }
      ledShow();
    } else {
      mixedRainboxIndex = 0;
    }
  }

  //BPM
  if (effect == Effect::bpm) {
    EVERY_N_MILLISECONDS(10) {
      nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
      {
        gHue++;
      }
    }
    EVERY_N_SECONDS(5) {
      targetPalette = CRGBPalette16(CHSV(random16(), 255, random16(128, 255)),
                                    CHSV(random16(), 255, random16(128, 255)),
                                    CHSV(random16(), 192, random16(128, 255)),
                                    CHSV(random16(), 255, random16(128, 255)));
    }
  }

  //FLASH AND FADE SUPPORT
  if (flash) {
    if (startFlash) {
      startFlash = false;
      flashStartTime = millis();
    }

    if ((millis() - flashStartTime) <= flashLength) {
      if ((millis() - flashStartTime) % 1000 <= 500) {
        setColor(flashRed, flashGreen, flashBlue);
      } else {
        setColor(0, 0, 0);
        // If you'd prefer the flashing to happen "on top of"
        // the current color, uncomment the next line.
        // setColor(realRed, realGreen, realBlue);
      }
    } else {
      flash = false;
      if (onbeforeflash) { //keeps light off after flash if light was originally off
        setColor(realRed, realGreen, realBlue);
      } else {
        stateOn = false;
        setColor(0, 0, 0);
#ifdef TARGET_GLOWWORMLUCIFERINFULL
        sendStatus();
#endif
        turnOffRelay();
      }
    }
  }

  if (startFade && effect == Effect::solid) {

    // If we don't want to fade, skip it.
    if (transitionTime == 0) {
      setColor(realRed, realGreen, realBlue);
      redVal = realRed;
      grnVal = realGreen;
      bluVal = realBlue;
      startFade = false;
    } else {
      loopCount = 0;
      stepR = calculateStep(redVal, realRed);
      stepG = calculateStep(grnVal, realGreen);
      stepB = calculateStep(bluVal, realBlue);
      inFade = true;
    }
  }

  if (inFade) {
    startFade = false;
    unsigned long now = millis();
    if (now - lastLoop > transitionTime) {
      if (loopCount <= 1020) {
        lastLoop = now;
        redVal = calculateVal(stepR, redVal, loopCount);
        grnVal = calculateVal(stepG, grnVal, loopCount);
        bluVal = calculateVal(stepB, bluVal, loopCount);
        if (effect == Effect::solid) {
          loopCount = 1020;
          setColor(redVal, grnVal, bluVal); // Write current values to LED pins
        }
        loopCount++;
      } else {
        inFade = false;
      }
    }
  }

}

/**
 * Main task for ESP32, pinned to CORE0
 */
#if defined(ESP32)

void feedTheDog(){
  // feed dog
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG0.wdt_feed=1;                       // feed dog
  TIMERG0.wdt_wprotect=0;                   // write protect
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG0.wdt_feed=1;                       // feed dog
  TIMERG0.wdt_wprotect=0;                   // write protect
}

/**
 * Pinned on CORE0, max performance with TCP
 * @param parameter
 */
void tcpTask(void * parameter) {
  while(true) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    sendSerialInfo();
    vTaskDelay(1);
#elif TARGET_GLOWWORMLUCIFERINFULL
    // If packet received...
    int packetSize = UDP.parsePacket();
    if (packetSize) {
      int len = UDP.read(packet, UDP_PACKET_SIZE);
      if (len > 0) {
        packet[len] = '\0';
        if (packetSize > 3) {
            fromStreamToStrip(packet, true);
        }
      }
    }
    vTaskDelay(1);
    EVERY_N_MILLISECONDS(50) {
      feedTheDog();
    }
    if (effect == Effect::GlowWormWifi) {
      mainLoop();
      vTaskDelay(1);
    }
    sendSerialInfo();
    vTaskDelay(1);
    if (firmwareUpgrade) {
      vTaskDelete(handleTcpTask);
    }
#endif
  }
}

/**
 * Pinned on CORE1, max performance with Serial
 * @param parameter
 */
void serialTask(void * parameter) {
  while(true) {
#ifdef TARGET_GLOWWORMLUCIFERINLIGHT
    mainLoop();
#elif TARGET_GLOWWORMLUCIFERINFULL
    if (effect != Effect::GlowWormWifi) {
      mainLoop();
    } else {
      delay(1000);
    }
    if (firmwareUpgrade) {
      vTaskDelete(handleSerialTask);
    }
#endif
  }
}

#endif

/**
 * Pinned on CORE1 on ESP32, max performance with Serial
 */
void loop() {

#if defined(ESP8266)
  mainLoop();
  if (firmwareUpgrade) {
    server.handleClient();
  }
#endif
#if defined(ESP32)
  // Upgrade is managed in single core mode, delete tasks pinned to CORE0 and CORE1
  if (firmwareUpgrade) {
    mainLoop();
    vTaskDelay(1);
    server.handleClient();
  } else {
    delay(1000);
  }
#endif

#ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (relayState && !stateOn) {
    turnOffRelay();
  }
#if defined(ESP8266)
  // If packet received...
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packet, UDP_PACKET_SIZE);
    if (len > 0) {
      packet[len] = '\0';
      if (packetSize > 3) {
        fromStreamToStrip(packet, true);
      }
    }
  }
#endif
#endif

}

/**
 * Turn ON the relay
 */
void turnOnRelay() {

  if (!relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
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
    digitalWrite(RELAY_PIN, LOW);
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
    Serial.printf("framerate:%s\n", helper.string2char(serialized(String((framerate > 0.5 ? framerate : 0),1))));
    Serial.printf("firmware:%s\n", "LIGHT");
#else
    Serial.printf("firmware:%s\n", "FULL");
    Serial.printf("mqttopic:%s\n", helper.string2char(topicInUse));
#endif
    Serial.printf("ver:%s\n", VERSION);
    Serial.printf("lednum:%d\n", dynamicLedNum);
#if defined(ESP32)
    Serial.printf("board:%s\n", "ESP32");
#elif defined(ESP8266)
    Serial.printf("board:%s\n", "ESP8266");
#endif
    Serial.printf("MAC:%s\n", helper.string2char(MAC));
    Serial.printf("gpio:%d\n", gpioInUse);
    Serial.printf("baudrate:%d\n", baudRateInUse);
    Serial.printf("effect:%d\n", effect);

  }

}

// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
  The program works like this:
  Imagine a crossfade that moves the red LED from 0-10,
    the green from 0-5, and the blue from 10 to 7, in
    ten steps.
    We'd want to count the 10 steps and increase or
    decrease color values in evenly stepped increments.
    Imagine a + indicates raising a value by 1, and a -
    equals lowering it. Our 10 step fade would look like:
    1 2 3 4 5 6 7 8 9 10
  R + + + + + + + + + +
  G   +   +   +   +   +
  B     -     -     -
  The red rises from 0 to 10 in ten steps, the green from
  0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
  In the real program, the color percentages are converted to
  0-255 values, and there are 1020 steps (255*4).
  To figure out how big a step there should be between one up- or
  down-tick of one of the LED values, we call calculateStep(),
  which calculates the absolute gap between the start and end values,
  and then divides that gap by 1020 to determine the size of the step
  between adjustments in the value.
*/
int calculateStep(int prevValue, int endValue) {

  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero,
    step = 1020 / step;          //   divide by 1020
  }

  return step;

}

/* The next function is calculateVal. When the loop value, i,
   reaches the step size appropriate for one of the
   colors, it increases or decreases the value of that color by 1.
   (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {

  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    } else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }

  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  } else if (val < 0) {
    val = 0;
  }

  return val;

}

void showleds() {

#if defined(ESP8266)
  delay(1);
#endif
#if defined(ESP32)
  vTaskDelay(1);
#endif
  if (stateOn) {
    ledShow();
  } else if (startFade) {
    setColor(0, 0, 0);
    startFade = false;
  }

}

// WS2812B LED Strip switches Red and Green
CRGB Scroll(int pos) {
  CRGB color (0,0,0);
  if(pos < 85) {
    color.g = 0;
    color.r = ((float)pos / 85.0f) * 255.0f;
    color.b = 255 - color.r;
  } else if(pos < 170) {
    color.g = ((float)(pos - 85) / 85.0f) * 255.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if(pos < 256) {
    color.b = ((float)(pos - 170) / 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
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
/**
 * Apply brightness correction on DMA mode
 * @param b red channel
 * @return corrected brightness
 */
uint8_t applyBrightnessCorrection(uint8_t c) {
  return (c && brightness) > 0 ? (c*((brightness*100)/255))/100 : c;
}
/**
 * Init LEDs
 */
void initLeds() {

#if defined(ESP32)
  Serial.println("Using DMA");
  cleanLEDs();
  ledsESP32 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1800KbpsMethod>(dynamicLedNum, gpioInUse); // and recreate with new count
  if (ledsESP32 == NULL) {
    Serial.println("OUT OF MEMORY");
  }
  while (!Serial); // wait for serial attach
  Serial.println();
  Serial.println("Initializing...");
  Serial.flush();
  ledsESP32->Begin();
  ledsESP32->Show();
#else
  if (gpioInUse == 3) {
    Serial.println("Using DMA");
    cleanLEDs();
    ledsDMA = new NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod >(dynamicLedNum, 3); // and recreate with new count
    if (ledsDMA == NULL) {
      Serial.println("OUT OF MEMORY");
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();
    ledsDMA->Begin();
    ledsDMA->Show();
  } else if (gpioInUse == 2) {
    Serial.println("Using UART");
    cleanLEDs();
    ledsUART = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod>(dynamicLedNum, 2); // and recreate with new count
    if (ledsUART == NULL) {
      Serial.println("OUT OF MEMORY");
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();
    ledsUART->Begin();
    ledsUART->Show();
  } else {
    Serial.println("Using Standard");
    cleanLEDs();
    ledsStandard = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBangWs2812xMethod >(dynamicLedNum, 5); // and recreate with new count
    if (ledsStandard == NULL) {
      Serial.println("OUT OF MEMORY");
    }
    while (!Serial); // wait for serial attach
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();
    ledsStandard->Begin();
    ledsStandard->Show();
  }
#endif

}

/**
 * Clean the LEDs before reinit
 */
void cleanLEDs() {

#if defined(ESP32)
  if (ledsESP32 != NULL) {
    delete ledsESP32; // delete the previous dynamically created strip
  }
#endif
#if defined(ESP8266)
  if (ledsDMA != NULL) {
    delete ledsDMA; // delete the previous dynamically created strip
  }
  if (ledsUART != NULL) {
    delete ledsUART; // delete the previous dynamically created strip
  }
  if (ledsStandard != NULL) {
    delete ledsStandard; // delete the previous dynamically created strip
  }
#endif

}

/**
 * Set pixel color
 * @param index LED num
 * @param r red channel
 * @param g green channel
 * @param b blu channel
 */
void setPixelColor(int index, uint8_t r, uint8_t g, uint8_t b) {

#if defined(ESP32)
  ledsESP32->SetPixelColor(index, RgbColor(applyWhiteTempRed(r),
                                           applyWhiteTempGreen(g),
                                           applyWhiteTempBlue(b)));
#else
  if (gpioInUse == 3) {
    ledsDMA->SetPixelColor(index, RgbColor(applyWhiteTempRed(r),
                                           applyWhiteTempGreen(g),
                                           applyWhiteTempBlue(b)));
  } else if (gpioInUse == 2) {
    ledsUART->SetPixelColor(index, RgbColor(applyWhiteTempRed(r),
                                            applyWhiteTempGreen(g),
                                            applyWhiteTempBlue(b)));
  } else {
    ledsStandard->SetPixelColor(index, RgbColor(applyWhiteTempRed(r),
                                                applyWhiteTempGreen(g),
                                                applyWhiteTempBlue(b)));
  }
#endif

}

/**
 * Show LEDs
 */
void ledShow() {

#if defined(ESP32)
  ledsESP32->Show();
#else
  if (gpioInUse == 3) {
    ledsDMA->Show();
  } else if (gpioInUse == 2) {
    ledsUART->Show();
  } else {
    ledsStandard->Show();
  }
#endif

}
