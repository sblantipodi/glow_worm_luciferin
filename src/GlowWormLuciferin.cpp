/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC

  Copyright (C) 2020  Davide Perini

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
   - WS2812B 5V LED Strip (95 LED)
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
struct PINUtil {
    template<uint8_t DYNAMIC_DATA_PIN = DATA_PIN>
    void init(int ledToUse) {
      FastLED.addLeds<CHIPSET, DYNAMIC_DATA_PIN, COLOR_ORDER>(leds, ledToUse);
    }
};

PINUtil pinUtil;

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
  // Bootsrap setup() with Wifi and MQTT functions
  bootstrapManager.bootstrapSetup(manageDisconnections, manageHardwareButton, callback);
  #endif

  // GPIO pin from configuration storage, overwrite the one saved during initial Arduino Bootstrapper config
  String gpioFromStorage = bootstrapManager.readValueFromFile(GPIO_FILENAME, GPIO_PARAM);
  if (!gpioFromStorage.isEmpty() && gpioFromStorage != ERROR && gpioFromStorage.toInt() != 0) {
    additionalParam = gpioFromStorage.toInt();
  }
  Serial.print(F("SAVED GPIO="));
  Serial.println(additionalParam);

  switch (additionalParam.toInt()) {
    case 2:
      gpioInUse = 2;
      pinUtil.init<2>(dynamicLedNum);
      break;
    case 16:
      gpioInUse = 16;
      pinUtil.init<16>(dynamicLedNum);
      break;
    default:
      gpioInUse = 5;
      additionalParam = gpioInUse;
      pinUtil.init<5>(dynamicLedNum);
      break;
  }
  Serial.print(F("GPIO IN USE="));
  Serial.println(gpioInUse);

  setupStripedPalette(CRGB::Red, CRGB::Red, CRGB::White, CRGB::White); //for CANDY CANE
  gPal = HeatColors_p; //for FIRE

  #if defined(ESP32)
  xTaskCreatePinnedToCore(
          mainTask,           /* Task function. */
          "mainTask",        /* name of task. */
          30000,                    /* Stack size of task */
          NULL,                     /* parameter of the task */
          1,                        /* priority of the task */
          NULL,                /* Task handle to keep track of created task */
          0);
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
  ESP.restart();

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
  #if defined(ESP8266)
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
  bootstrapManager.writeToLittleFS(baudrateDoc, BAUDRATE_FILENAME);
  #endif
  #if defined(ESP32)
  DynamicJsonDocument baudrateDoc(1024);
  baudrateDoc[BAUDRATE_PARAM] = baudRateInUse;
  bootstrapManager.writeToSPIFFS(baudrateDoc, BAUDRATE_FILENAME);
  #endif
  delay(20);
  ESP.restart();

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

  bootstrapManager.subscribe(LIGHT_SET_TOPIC);
  bootstrapManager.subscribe(helper.string2char(STREAM_TOPIC), 0);
  bootstrapManager.subscribe(CMND_AMBI_REBOOT);
  bootstrapManager.subscribe(UPDATE_STATE_TOPIC);
  bootstrapManager.subscribe(GPIO_TOPIC);
  bootstrapManager.subscribe(UNSUBSCRIBE_TOPIC);
  bootstrapManager.subscribe(BAUDRATE_TOPIC);

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

  if (STREAM_TOPIC.equals(topic)) {

    if (effect == Effect::GlowWormWifi) {
      bootstrapManager.jsonDoc.clear();
      deserializeJson(bootstrapManager.jsonDoc, payload);
      int numLedFromLuciferin = bootstrapManager.jsonDoc[LED_NUM_PARAM];
      if (numLedFromLuciferin == 0) {
        effect = Effect::solid;
      } else {
        if (dynamicLedNum != numLedFromLuciferin) {
          setNumLed(numLedFromLuciferin);
        }
        // (leds, 0, (dynamicLedNum) * sizeof(struct CRGB));
        JsonArray stream = bootstrapManager.jsonDoc["stream"];
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
            part = bootstrapManager.jsonDoc["part"];
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
            if (dynamicLedNum < 380) {
              FastLED.show();
            }
          } else {
            int j = 0;
            for (int16_t i = SECOND_CHUNK; i >= SECOND_CHUNK && i < NUM_LEDS; i++) {
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
        if ((dynamicLedNum < FIRST_CHUNK) || (dynamicLedNum < SECOND_CHUNK && part == 2) || (part == 3)) {
          framerateCounter++;
        }
        #endif
        lastStream = millis();
      }
    }

  } else if (effect != Effect::GlowWormWifi) {

    bootstrapManager.jsonDoc.clear();
    bootstrapManager.parseQueueMsg(topic, payload, length);
    if (strcmp(topic, CMND_AMBI_REBOOT) == 0) {
      processGlowWormLuciferinRebootCmnd(bootstrapManager.jsonDoc);
    } else if (strcmp(topic, LIGHT_SET_TOPIC) == 0) {
      processJson(bootstrapManager.jsonDoc);
    } else if (strcmp(topic, UPDATE_STATE_TOPIC) == 0) {
      processUpdate(bootstrapManager.jsonDoc);
    } else if (strcmp(topic, GPIO_TOPIC) == 0) {
      processGPIO(bootstrapManager.jsonDoc);
    } else if (strcmp(topic, BAUDRATE_TOPIC) == 0) {
      processBaudrate(bootstrapManager.jsonDoc);
    } else if (strcmp(topic, UNSUBSCRIBE_TOPIC) == 0) {
      processUnSubscribeStream(bootstrapManager.jsonDoc);
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
 * Process GPIO message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processGPIO(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json.containsKey(GPIO_PARAM)) {
    int gpio = (int) json[GPIO_PARAM];
    String macToUpdate = json["MAC"];
    Serial.println(macToUpdate);
    Serial.println(MAC);
    if (gpio != 0 && gpioInUse != gpio ) {
      setGpio(gpio);
    }
  }
  return true;

}

/**
 * Process baudrate message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processBaudrate(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json.containsKey(BAUDRATE_PARAM)) {
    int baudrate = (int) json[BAUDRATE_PARAM];
    String macToUpdate = json["MAC"];
    Serial.println(macToUpdate);
    Serial.println(MAC);
    if (baudrate != 0 && baudRateInUse != baudrate && macToUpdate == MAC) {
      setBaudRate(baudrate);
    }
  }
  return true;

}

/**
 * Unsubscribe from the stream topic, we will use a specific topic for this instance
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processUnSubscribeStream(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json.containsKey("instance")) {
    String instance = json["instance"];
    String manager = json["manager"];
    if (manager.equals(deviceName)) {
      bootstrapManager.unsubscribe(helper.string2char(STREAM_TOPIC));
      STREAM_TOPIC = BASE_STREAM_TOPIC + instance;
      effect = Effect::GlowWormWifi;
      stateOn = true;
      bootstrapManager.subscribe(helper.string2char(STREAM_TOPIC), 0);
    }
  }
  return true;

}

/**
 * Process JSON message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool processJson(StaticJsonDocument<BUFFER_SIZE> json) {

  lastLedUpdate = millis();

  if (json.containsKey("state")) {
    String state = json["state"];
    if (state == ON_CMD) {
      stateOn = true;
    } else if (state == OFF_CMD) {
      stateOn = false;
      onbeforeflash = false;
    }
  }

  // If "flash" is included, treat RGB and brightness differently
  if (json.containsKey("flash")) {
    flashLength = (int) json["flash"] * 1000;

    if (json.containsKey("brightness")) {
      flashBrightness = json["brightness"];
    } else {
      flashBrightness = brightness;
    }

    if (json.containsKey("color")) {
      flashRed = json["color"]["r"];
      flashGreen = json["color"]["g"];
      flashBlue = json["color"]["b"];
    } else {
      flashRed = red;
      flashGreen = green;
      flashBlue = blue;
    }

    if (json.containsKey("transition")) {
      transitionTime = json["transition"];
    } else if (effect == Effect::solid) {
      transitionTime = 0;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
    flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
    flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);
    flash = true;
    startFlash = true;

  } else { // Not flashing

    flash = false;
    if (stateOn) {   //if the light is turned on and the light isn't flashing
      onbeforeflash = true;
    }

    if (json.containsKey("color")) {
      red = json["color"]["r"];
      green = json["color"]["g"];
      blue = json["color"]["b"];
    }

    if (json.containsKey("brightness")) {
      brightness = json["brightness"];
      FastLED.setBrightness(brightness);
    }

    if (json.containsKey("transition")) {
      transitionTime = json["transition"];
    } else if (effect == Effect::solid) {
      transitionTime = 0;
    }

  }

  if (json.containsKey("effect")) {
    JsonVariant requestedEffect = json["effect"];
    if (requestedEffect == "GlowWorm") {
      effect = Effect::GlowWorm;
      FastLED.setBrightness(brightness);
      lastLedUpdate = millis();
    } else if (requestedEffect == "GlowWormWifi") {
      effect = Effect::GlowWormWifi;
      FastLED.setBrightness(brightness);
      lastStream = millis();
    } else if (requestedEffect == "bpm") effect = Effect::bpm;
    else if (requestedEffect == "rainbow") effect = Effect::rainbow;
    else if (requestedEffect == "solid rainbow") effect = Effect::solid_rainbow;
    else if (requestedEffect == "mixed rainbow") effect = Effect::mixed_rainbow;
    else {
      effect = Effect::solid;
    }
    statusSent = false;
    twinklecounter = 0; //manage twinklecounter
  }

  return true;

}

/**
 * Send microcontroller state
 */
void sendStatus() {
  // Skip JSON framework for lighter processing during the stream
  if (statusSent && (effect == Effect::GlowWorm || effect == Effect::GlowWormWifi)) {
    bootstrapManager.publish(FPS_TOPIC, helper.string2char("{\"deviceName\":\""+deviceName+"\",\"MAC\":\""+MAC+"\",\"lednum\":\""+dynamicLedNum+"\",\"framerate\":\""+framerate+"\"}"), false);
  } else {
    JsonObject root = bootstrapManager.getJsonObject();
    JsonObject color = root.createNestedObject("color");
    root["state"] = (stateOn) ? ON_CMD : OFF_CMD;
    color["r"] = red;
    color["g"] = green;
    color["b"] = blue;
    root["brightness"] = brightness;
    switch (effect) {
      case Effect::GlowWormWifi:
        root["effect"] = "GlowWormWifi";
        statusSent = true;
        break;
      case Effect::GlowWorm:
        root["effect"] = "GlowWorm";
        statusSent = true;
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
    root["gpio"] = additionalParam;

    if (effect == Effect::solid && !stateOn) {
      setColor(0, 0, 0);
    }

    // This topic should be retained, we don't want unknown values on battery voltage or wifi signal
    bootstrapManager.publish(LIGHT_STATE_TOPIC, root, true);
  }

  #if defined(ESP32)
  delay(1);
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
bool processUpdate(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json.containsKey(F("update"))) {

    Serial.println(F("Starting web server"));
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        bootstrapManager.publish(UPDATE_RESULT_STATE_TOPIC, helper.string2char(deviceName), false);
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
bool processGlowWormLuciferinRebootCmnd(StaticJsonDocument<BUFFER_SIZE> json) {

  if (json[VALUE] == OFF_CMD) {
    stateOn = false;
    sendStatus();
    delay(1500);
    ESP.restart();
  }
  return true;

}

#endif

/**
 * Set led strip color
 * @param inR red color
 * @param inG green color
 * @param inB blu color
 */
void setColor(int inR, int inG, int inB) {

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].red = inR;
    leds[i].green = inG;
    leds[i].blue = inB;
  }

  FastLED.show();

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
    }
  }
  #endif
  #if defined(ESP8266)
  sendSerialInfo();
  #endif

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
    fireflyEffect = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    chk = serialRead();

    if (!breakLoop && (chk != (hi ^ lo ^ loSecondPart ^ usbBrightness ^ gpio ^ baudRate ^ fireflyEffect ^ 0x55))) {
      i = 0;
      goto waitLoop;
    }

    if (usbBrightness != brightness) {
      FastLED.setBrightness(usbBrightness);
      brightness = usbBrightness;
    }

    if (gpio != 0 && gpioInUse != gpio && (gpio == 2 || gpio == 5 || gpio == 16)) {
      setGpio(gpio);
    }

    int numLedFromLuciferin = lo + loSecondPart + 1;
    if (dynamicLedNum != numLedFromLuciferin && numLedFromLuciferin < NUM_LEDS) {
      setNumLed(numLedFromLuciferin);
    }

    if (baudRate != 0 && baudRateInUse != baudRate && (baudRate >= 1 && baudRate <= 7)) {
      setBaudRate(baudRate);
    }

    if (fireflyEffect != 0 && fireflyEffectInUse != fireflyEffect) {
      fireflyEffectInUse = fireflyEffect;
    }

    // memset(leds, 0, (numLedFromLuciferin) * sizeof(struct CRGB));
    for (uint16_t i = 0; i < (numLedFromLuciferin); i++) {
      byte r, g, b;
      while (!breakLoop && !Serial.available()) checkConnection();
      r = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      g = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      b = serialRead();
      leds[i].r = r;
      leds[i].g = g;
      leds[i].b = b;
    }
    lastLedUpdate = millis();
    framerateCounter++;
    FastLED.show();

    // Flush serial buffer
    while (!breakLoop && Serial.available() > 0) {
      serialRead();
    }
  #ifdef TARGET_GLOWWORMLUCIFERINFULL
  }
  #endif
  breakLoop = false;
  #ifdef TARGET_GLOWWORMLUCIFERINFULL

  //EFFECT BPM
  if (effect == Effect::bpm) {
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0) {
      transitionTime = 30;
    }
    showleds();
  }

  //EFFECT RAINBOW
  if (effect == Effect::rainbow) {
    // FastLED's built-in rainbow generator
    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
    if (transitionTime == 0) {
      transitionTime = 130;
    }
    showleds();
  }

  //SOLID RAINBOW
  if (effect == Effect::solid_rainbow) {
    // FastLED's built-in rainbow generator
    thishue++;
    fill_solid(leds, NUM_LEDS, CHSV(thishue, 255, 255));
    if (transitionTime == 0) {
      transitionTime = 40;
    }
    if (transitionTime < 130) {
      delay(130 - transitionTime);
    }
    showleds();
  }

  //EFFECT mixed rainbow
  if (effect == Effect::mixed_rainbow) {
    for(int j = 0; j < 256; j++) {
      for(int i = 0; i < dynamicLedNum; i++) {
        leds[i] = Scroll((i * 256 / dynamicLedNum + j) % 256);
        #ifdef TARGET_GLOWWORMLUCIFERINFULL
        checkConnection();
        #endif
      }
      FastLED.show();
    }
  }

  //EVERY 10 MILLISECONDS
  EVERY_N_MILLISECONDS(10) {

    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
    {
      gHue++;
    }



  }

  // EVERY 5 SECONDS
  EVERY_N_SECONDS(5) {

    targetPalette = CRGBPalette16(CHSV(random16(), 255, random16(128, 255)), CHSV(random16(), 255, random16(128, 255)),
                                  CHSV(random16(), 192, random16(128, 255)), CHSV(random16(), 255, random16(128, 255)));

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
        sendStatus();
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
          setColor(redVal, grnVal, bluVal); // Write current values to LED pins
        }
        loopCount++;
      } else {
        inFade = false;
      }
    }
  }
#endif

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
}

void mainTask(void * parameter) {

  while(true) {
    mainLoop();
    EVERY_N_MILLISECONDS(1000) {
      feedTheDog();
    }
  }

}
#endif

void loop() {

  #if defined(ESP8266)
  mainLoop();
  #endif
  if (firmwareUpgrade) {
    server.handleClient();
  }
  #if defined(ESP32)
  EVERY_N_MILLISECONDS(1000) {
    feedTheDog();
  }
  sendSerialInfo();
  #endif

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
    #endif
    Serial.printf("ver:%s\n", VERSION);
    Serial.printf("lednum:%d\n", dynamicLedNum);
    #if defined(ESP32)
    Serial.printf("board:%s\n", "ESP32");
    #elif defined(ESP8266)
    Serial.printf("board:%s\n", "ESP8266");
    #endif
    Serial.printf("MAC:%s\n", helper.string2char(MAC));
    Serial.printf("gpio:%s\n", helper.string2char(additionalParam));
    Serial.printf("baudrate:%d\n", baudRateInUse);

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

void setupStripedPalette(CRGB A, CRGB AB, CRGB B, CRGB BA) {

  currentPalettestriped = CRGBPalette16(
          A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
  );

}

void fadeall() {

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);  //for CYCLon
  }

}

void Fire2012WithPalette() {

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random16(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random16() < SPARKING) {
    int y = random16(7);
    heat[y] = qadd8(heat[y], random16(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(gPal, colorindex);
    int pixelnumber;
    if (gReverseDirection) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }

}

void addGlitter(fract8 chanceOfGlitter) {

  if (random16() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }

}

void addGlitterColor(fract8 chanceOfGlitter, int red, int green, int blue) {

  if (random16() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB(red, green, blue);
  }

}

void showleds() {

  delay(1);
  if (stateOn) {
    FastLED.setBrightness(brightness);  //EXECUTE EFFECT COLOR
    FastLED.show();
    if (transitionTime > 0 && transitionTime < 130) {  //Sets animation speed based on receieved value
      FastLED.delay(1000 / transitionTime);
      //delay(10*transitionTime);
    }
  } else if (startFade) {
    setColor(0, 0, 0);
    startFade = false;
  }

}

void temp2rgb(unsigned int kelvin) {

  int tmp_internal = kelvin / 100.0;

  // red
  if (tmp_internal <= 66) {
    red = 255;
  } else {
    float tmp_red = 329.698727446 * pow(tmp_internal - 60, -0.1332047592);
    if (tmp_red < 0) {
      red = 0;
    } else if (tmp_red > 255) {
      red = 255;
    } else {
      red = tmp_red;
    }
  }

  // green
  if (tmp_internal <= 66) {
    float tmp_green = 99.4708025861 * log(tmp_internal) - 161.1195681661;
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  } else {
    float tmp_green = 288.1221695283 * pow(tmp_internal - 60, -0.0755148492);
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  }

  // blue
  if (tmp_internal >= 66) {
    blue = 255;
  } else if (tmp_internal <= 19) {
    blue = 0;
  } else {
    float tmp_blue = 138.5177312231 * log(tmp_internal - 10) - 305.0447927307;
    if (tmp_blue < 0) {
      blue = 0;
    } else if (tmp_blue > 255) {
      blue = 255;
    } else {
      blue = tmp_blue;
    }
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
