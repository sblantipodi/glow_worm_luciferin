/*
  NetManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include "NetManager.h"

uint16_t NetManager::part = 1;
[[maybe_unused]] boolean NetManager::firmwareUpgrade = false;
size_t NetManager::updateSize = 0;
String NetManager::fpsData;



/**
 * Parse UDP packet
 */
void NetManager::getUDPStream() {
  yield();
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
    broadcastUDP.read(packetBroadcast, UDP_BR_MAX_BUFFER_SIZE);
    packetBroadcast[packetSizeBroadcast] = '\0';
    char * dn;
    char * dnStatic;
    dn = strstr (packetBroadcast, DN);
    dnStatic = strstr (packetBroadcast, DNStatic);
    if (dn || dnStatic) {
      if (dnStatic) {
        for (uint16_t dnIdx = 0; dnIdx < packetSizeBroadcast; dnIdx++) {
          dname[dnIdx] = packetBroadcast[dnIdx + strlen(DNStatic)];
        }
      } else {
        for (uint16_t dnIdx = 0; dnIdx < packetSizeBroadcast; dnIdx++) {
          dname[dnIdx] = packetBroadcast[dnIdx + strlen(DN)];
        }
      }
      if (!remoteIpForUdp.toString().equals(broadcastUDP.remoteIP().toString())
        && ((strcmp(dname, deviceName.c_str()) == 0) || (strcmp(dname, microcontrollerIP.c_str()) == 0))) {
        remoteIpForUdp = broadcastUDP.remoteIP();
        Serial.println(F("-> Setting IP to use <-"));
        Serial.println(remoteIpForUdp.toString());
      }
    } else {
#if defined(ESP8266)
      if (!netManager.remoteIpForUdpBroadcast.isSet() || netManager.remoteIpForUdpBroadcast.toString().equals(remoteIpForUdp.toString())) {
#elif defined(ARDUINO_ARCH_ESP32)
      if (netManager.remoteIpForUdpBroadcast.toString().equals(F("0.0.0.0")) || netManager.remoteIpForUdpBroadcast.toString().equals(remoteIpForUdp.toString())) {
#endif
        char *p;
        p = strstr(packetBroadcast, PING);
        if (p) {
          for (uint16_t brIdx = 0; brIdx < packetSizeBroadcast; brIdx++) {
            broadCastAddress[brIdx] = packetBroadcast[brIdx + strlen(PING)];
          }
          if (!remoteIpForUdpBroadcast.toString().equals(broadCastAddress)) {
            remoteIpForUdpBroadcast.fromString(broadCastAddress);
            Serial.println(F("-> Setting Broadcast IP to use <-"));
            Serial.println(remoteIpForUdpBroadcast.toString());
          }
        }
      }
    }
  }
}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void NetManager::fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]) {
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
  lastUdpMsgReceived = millis();
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  uint16_t numLedFromLuciferin = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  uint8_t audioBrightness = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  if (brightness != audioBrightness && !ldrEnabled) {
    brightness = audioBrightness;
  }
  uint8_t chunkTot, chunkNum;
  chunkTot = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  chunkNum = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  index = UDP_CHUNK_SIZE * chunkNum;
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      LedManager::setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != nullptr) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      if (ldrInterval != 0 && ldrEnabled && ldrReading && ldrTurnOff) {
        ledManager.setPixelColor(index, 0, 0, 0);
      } else {
        ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
      }
      index++;
      ptr = strtok_r(nullptr, delimiters, &saveptr);
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
void NetManager::manageDisconnections() {
  Serial.print(F("managing disconnections..."));
  if (wifiReconnectAttemp > 10 && wifiReconnectAttemp <= 20) {
    disconnectionTime = millis();
    disconnectionResetEnable = true;
  }
  if ((mqttReconnectAttemp > 10 && mqttReconnectAttemp <= 20) && (mqttIP.length() > 0)) {
    disconnectionTime = millis();
    disconnectionResetEnable = true;
  }
  if (disconnectionResetEnable && !builtInLedStatus) {
    builtInLedStatus = true;
    LedManager::manageBuiltInLed(0, 125, 255);
  }
  if (millis() - disconnectionTime > (secondsBeforeReset * 2)) {
    disconnectionTime = millis();
    ledManager.stateOn = true;
    effect = Effect::solid;
    String ap = bootstrapManager.readValueFromFile(AP_FILENAME, AP_PARAM);
    if ((ap.isEmpty() || ap == ERROR) || (!ap.isEmpty() && ap != ERROR && ap.toInt() != 0)) {
      JsonDocument asDoc;
      asDoc[AP_PARAM] = 0;
      BootstrapManager::writeToLittleFS(asDoc, AP_FILENAME);
    }
    LedManager::manageBuiltInLed(0, 0, 0);
    LedManager::setColor(0, 0, 0);
  } else if ((millis() - disconnectionTime > secondsBeforeReset) && disconnectionResetEnable) {
    resetLedStatus = true;
    disconnectionResetEnable = false;
    ledManager.stateOn = true;
    effect = Effect::solid;
    String ap = bootstrapManager.readValueFromFile(AP_FILENAME, AP_PARAM);
    if ((ap.isEmpty() || ap == ERROR) || (!ap.isEmpty() && ap != ERROR && ap.toInt() != 10)) {
      JsonDocument asDoc;
      asDoc[AP_PARAM] = 10;
      BootstrapManager::writeToLittleFS(asDoc, AP_FILENAME);
    }
    LedManager::manageBuiltInLed(255, 0, 0);
    LedManager::setColorLoop(255, 0, 0);
  }
}

/**
 * MQTT SUBSCRIPTIONS
 */
void NetManager::manageQueueSubscription() {
  // Note: Add another topic subscription can cause performance issues on ESP8266
  // Double check it with 60FPS, 100 LEDs, with MQTT enabled.
  BootstrapManager::subscribe(netManager.lightSetTopic.c_str(), 1);
  BootstrapManager::subscribe(netManager.cmndReboot.c_str());
  BootstrapManager::subscribe(netManager.updateStateTopic.c_str());
  BootstrapManager::subscribe(netManager.firmwareConfigTopic.c_str());
  BootstrapManager::subscribe(netManager.effectToGw.c_str());
  // TODO remove subscription to topic that doesn't need MQTT, some topics can be managed via HTTP only
  BootstrapManager::subscribe(netManager.streamTopic.c_str(), 0);
  BootstrapManager::subscribe(netManager.unsubscribeTopic.c_str());
  apFileRead = false;
}

/**
 * Unsubscribe from the default MQTT topic
 */
void NetManager::swapTopicUnsubscribe() {
  // No firmwareConfigTopic unsubscribe because that topic needs MAC, no need to swap topic
  BootstrapManager::unsubscribe(netManager.lightSetTopic.c_str());
  BootstrapManager::unsubscribe(netManager.effectToGw.c_str());
  BootstrapManager::unsubscribe(netManager.streamTopic.c_str());
  BootstrapManager::unsubscribe(netManager.cmndReboot.c_str());
  BootstrapManager::unsubscribe(netManager.updateStateTopic.c_str());
  BootstrapManager::unsubscribe(netManager.unsubscribeTopic.c_str());
}

/**
 * Swap MQTT topi with the custom one
 * @param customtopic custom MQTT topic to use, received by Firefly Luciferin
 */
void NetManager::swapTopicReplace(const String &customtopic) {
  // No firmwareConfigTopic unsubscribe because that topic needs MAC, no need to swap topic
  netManager.lightStateTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.effectToGw.replace(netManager.BASE_TOPIC, customtopic);
  netManager.effectToFw.replace(netManager.BASE_TOPIC, customtopic);
  netManager.updateStateTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.updateResultStateTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.lightSetTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.baseStreamTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.streamTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.unsubscribeTopic.replace(netManager.BASE_TOPIC, customtopic);
  netManager.cmndReboot.replace(netManager.BASE_TOPIC, customtopic);
}

/**
 * Subscribe to custom MQTT topic
 */
void NetManager::swapTopicSubscribe() {
  // No firmwareConfigTopic unsubscribe because that topic needs MAC, no need to swap topic
  BootstrapManager::subscribe(netManager.lightSetTopic.c_str(), 1);
  BootstrapManager::subscribe(netManager.effectToGw.c_str());
  BootstrapManager::subscribe(netManager.streamTopic.c_str(), 0);
  BootstrapManager::subscribe(netManager.cmndReboot.c_str());
  BootstrapManager::subscribe(netManager.updateStateTopic.c_str());
  BootstrapManager::subscribe(netManager.unsubscribeTopic.c_str());
}

/**
 * List on HTTP GET
 */
void NetManager::listenOnHttpGet() {
  server.on(F("/"), []() {
      stopUDP();
      server.send(200, F("text/html"), settingsPage);
      startUDP();
  });
  server.on(F("/prefs"), [this]() {
      prefsData = F("{\"VERSION\":\"");
      prefsData += VERSION;
      prefsData += F("\",\"cp\":\"");
      prefsData += ledManager.red;
      prefsData += F(",");
      prefsData += ledManager.green;
      prefsData += F(",");
      prefsData += ledManager.blue;
      prefsData += F("\",\"toggle\":\"");
      prefsData += ledManager.stateOn;
      prefsData += F("\",\"effect\":\"");
      prefsData += Globals::effectToString(effect);
      prefsData += F("\",\"ffeffect\":\"");
      prefsData += ffeffect;
      prefsData += F("\",\"whiteTemp\":\"");
      prefsData += whiteTempInUse;
      prefsData += F("\",\"brightness\":\"");
      prefsData += brightness;
      prefsData += F("\",\"wifi\":\"");
      prefsData += BootstrapManager::getWifiQuality();
      prefsData += F("\",\"framerate\":\"");
      prefsData += framerate;
      prefsData += F("\",\"autosave\":\"");
      prefsData += autoSave;
      if (ldrEnabled) {
        prefsData += F("\",\"ldr\":\"");
        prefsData += ((ldrValue * 100) / ldrDivider);
      }
      if (!mqttConnected && mqttIP.length() > 0) {
        prefsData += F("\",\"mqttError\":\"");
        prefsData += !mqttConnected;
      }
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(F("/getLdr"), [this]() {
      prefsData = F("{\"ldrEnabled\":\"");
      prefsData += ldrEnabled;
      prefsData += F("\",\"ldrInterval\":\"");
      prefsData += ldrInterval;
      prefsData += F("\",\"ldrTurnOff\":\"");
      prefsData += ldrTurnOff;
      prefsData += F("\",\"ldrMin\":\"");
      prefsData += ldrMin;
      prefsData += F("\",\"relayPin\":\"");
      prefsData += relayPin;
      prefsData += F("\",\"relInv\":\"");
      prefsData += relInv;
      prefsData += F("\",\"sbPin\":\"");
      prefsData += sbPin;
      prefsData += F("\",\"ldrPin\":\"");
      prefsData += ldrPin;
      prefsData += F("\",\"ledBuiltin\":\"");
      prefsData += ledBuiltin;
      prefsData += F("\",\"ldrMax\":\"");
      if (ldrEnabled) {
        prefsData += ((ldrValue * 100) / ldrDivider);
      } else {
        prefsData += 0;
      }
      if (!mqttConnected && mqttIP.length() > 0) {
        prefsData += F("\",\"mqttError\":\"");
        prefsData += !mqttConnected;
      }
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
  });
  server.on(F("/setldr"), []() {
      stopUDP();
      server.send(200, F("text/html"), setLdrPage);
      startUDP();
  });
  server.on(F("/setAutoSave"), []() {
      stopUDP();
      autoSave = server.arg(F("autosave")).toInt();
      JsonDocument asDoc;
      asDoc[F("autosave")] = autoSave;
      BootstrapManager::writeToLittleFS(asDoc, AUTO_SAVE_FILENAME);
      delay(20);
      server.send(200, F("text/html"), F("Stored"));
      startUDP();
  });
  server.on(F("/ldr"), []() {
      httpCallback(processLDR);
  });
  server.on(("/" + netManager.lightSetTopic).c_str(), []() {
      httpCallback(nullptr);
      setLeds();
      setColor();
  });
  server.on(F("/set"), []() {
      httpCallback(nullptr);
      setLeds();
      setColor();
  });
  server.on(("/" + netManager.cmndReboot).c_str(), []() {
      httpCallback(processGlowWormLuciferinRebootCmnd);
  });
  server.on(("/" + netManager.updateStateTopic).c_str(), []() {
      httpCallback(processUpdate);
  });
  server.on(("/" + netManager.unsubscribeTopic).c_str(), []() {
      httpCallback(processUnSubscribeStream);
  });
  server.on(("/" + netManager.firmwareConfigTopic).c_str(), []() {
      httpCallback(processFirmwareConfig);
  });
  server.onNotFound([]() {
      server.send(404, F("text/plain"), ("Glow Worm Luciferin: Uri not found ") + server.uri());
  });
  manageAPSetting(false);

  server.begin();
}

/**
 * Manage AP Settings
 */
void NetManager::manageAPSetting(bool isSettingRoot) {
  if (isSettingRoot) {
    server.on(F("/"), []() {
        server.send(200, F("text/html"), setSettingsPageOffline);
    });
  } else {
    server.on(F("/setsettings"), []() {
        stopUDP();
        server.send(200, F("text/html"), setSettingsPage);
        startUDP();
    });
  }
  server.on(F("/getsettings"), [this]() {
      stopUDP();
      prefsData = F("{\"deviceName\":\"");
      prefsData += deviceName;
      prefsData += F("\",\"dhcp\":\"");
      prefsData += dhcpInUse;
      prefsData += F("\",\"ip\":\"");
      prefsData += microcontrollerIP;
      prefsData += F("\",\"dhcp\":\"");
      prefsData += dhcpInUse;
      prefsData += F("\",\"ethd\":\"");
      prefsData += ethd;
      prefsData += F("\",\"mqttuser\":\"");
      prefsData += mqttuser;
      prefsData += F("\",\"mqttIp\":\"");
      prefsData += mqttIP;
      prefsData += F("\",\"mqttpass\":\"");
      prefsData += mqttpass;
      prefsData += F("\",\"mqttPort\":\"");
      prefsData += mqttPort;
      prefsData += F("\",\"mqttTopic\":\"");
      prefsData += netManager.topicInUse;
      prefsData += F("\",\"lednum\":\"");
      prefsData += ledManager.dynamicLedNum;
      prefsData += F("\",\"gpio\":\"");
      prefsData += gpioInUse;
      prefsData += F("\",\"gpioClock\":\"");
      prefsData += gpioClockInUse;
      prefsData += F("\",\"colorMode\":\"");
      prefsData += colorMode;
      prefsData += F("\",\"colorOrder\":\"");
      prefsData += colorOrder;
      prefsData += F("\",\"br\":\"");
      prefsData += baudRateInUse;
      prefsData += F("\",\"ssid\":\"");
      prefsData += qsid.c_str();
      if (!mqttConnected && mqttIP.length() > 0) {
        prefsData += F("\",\"mqttError\":\"");
        prefsData += !mqttConnected;
      }
#if defined(ARDUINO_ARCH_ESP32)
      if (ethd >= spiStartIdx) {
        if (ethd == spiStartIdx) {
          prefsData += F("\",\"mosi\":\"");
          prefsData += mosi;
          prefsData += F("\",\"miso\":\"");
          prefsData += miso;
          prefsData += F("\",\"sclk\":\"");
          prefsData += sclk;
          prefsData += F("\",\"cs\":\"");
          prefsData += cs;
        }
      }
#endif
      prefsData += F("\"}");
      server.send(200, F("application/json"), prefsData);
      startUDP();
  });
  server.on(F("/setting"), []() {
      stopUDP();
      httpCallback(processFirmwareConfigWithReboot);
  });
}

/**
 * Set color
 */
void NetManager::setColor() {
  if (ledManager.stateOn) {
    LedManager::setColor(map(ledManager.red, 0, 255, 0, brightness), map(ledManager.green, 0, 255, 0, brightness),
                         map(ledManager.blue, 0, 255, 0, brightness));
  } else {
    LedManager::setColor(0, 0, 0);
  }
}

/**
 * Set LEDs state, used by HTTP and MQTT requests
 */
void NetManager::setLeds() {
  String requestedEffect = bootstrapManager.jsonDoc[F("effect")];
  ffeffect = bootstrapManager.jsonDoc[F("effect")].as<String>();
  if (requestedEffect == F("GlowWormWifi") || requestedEffect == F("GlowWormWifi")
      || requestedEffect.indexOf("Music") > -1 || requestedEffect.indexOf("Bias") > -1) {
    bootstrapManager.jsonDoc[F("effect")].set(F("GlowWormWifi"));
    requestedEffect = "GlowWormWifi";
  }
  processJson();
  if (mqttIP.length() > 0) {
    if (requestedEffect == F("GlowWormWifi") || requestedEffect == F("GlowWormWifi")) {
      BootstrapManager::publish(netManager.effectToFw.c_str(), ffeffect.c_str(), false);
    } else {
      if (ledManager.stateOn) {
        BootstrapManager::publish(netManager.effectToFw.c_str(), ffeffect.c_str(), false);
      } else {
        BootstrapManager::publish(netManager.effectToFw.c_str(), OFF_CMD.c_str(), false);
      }
      framerate = framerateCounter = 0;
    }
  } else {
#if defined(ESP8266)
    if (netManager.remoteIpForUdp.isSet()) {
#elif defined(ARDUINO_ARCH_ESP32)
    if (!netManager.remoteIpForUdp.toString().equals(F("0.0.0.0"))) {
#endif
      netManager.broadcastUDP.beginPacket(netManager.remoteIpForUdp, UDP_BROADCAST_PORT);
      if (requestedEffect == F("GlowWorm") || requestedEffect == F("GlowWormWifi")) {
        netManager.broadcastUDP.print(ffeffect.c_str());
      } else {
        netManager.broadcastUDP.print(netManager.STOP_FF);
        framerate = framerateCounter = 0;
      }
      netManager.broadcastUDP.endPacket();
    }
  }
}

/**
 * Stop UDP broadcast while serving pages
 */
void NetManager::stopUDP() {
  netManager.UDP.stop();
  netManager.servingWebPages = true;
  delay(10);
}

/*
 * Start UDP broadcast while serving pages
 */
void NetManager::startUDP() {
  delay(10);
  netManager.servingWebPages = false;
  netManager.UDP.begin(UDP_PORT);
}

/**
 * MANAGE HARDWARE BUTTON
 */
void NetManager::manageHardwareButton() {
  // no hardware button at the moment
}

/**
 * START CALLBACK
 * @param topic MQTT topic
 * @param payload MQTT payload
 * @param length MQTT message length
 */
void NetManager::callback(char *topic, byte *payload, unsigned int length) {
  if (netManager.streamTopic.equals(topic)) {
    if (effect == Effect::GlowWormWifi) {
      fromMqttStreamToStrip(reinterpret_cast<char *>(payload));
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    bootstrapManager.parseQueueMsg(topic, payload, length);
    if (netManager.cmndReboot.equals(topic)) {
      processGlowWormLuciferinRebootCmnd();
    } else if (netManager.lightSetTopic.equals(topic)) {
      processJson();
    } else if (netManager.effectToGw.equals(topic)) {
      setLeds();
    } else if (netManager.updateStateTopic.equals(topic)) {
      processMqttUpdate();
    } else if (netManager.firmwareConfigTopic.equals(topic)) {
      processFirmwareConfig();
    } else if (netManager.unsubscribeTopic.equals(topic)) {
      processUnSubscribeStream();
    }
    if (ledManager.stateOn) {
      LedManager::setColor(map(ledManager.red, 0, 255, 0, brightness), map(ledManager.green, 0, 255, 0, brightness),
                           map(ledManager.blue, 0, 255, 0, brightness));
    } else {
      LedManager::setColor(0, 0, 0);
    }
  }
}

/**
 * Execute callback from HTTP payload
 * @param callback to execute using HTTP payload
 */
void NetManager::httpCallback(bool (*callback)()) {
  bootstrapManager.jsonDoc.clear();
  String payload = server.arg(F("payload"));
  bootstrapManager.parseHttpMsg(payload, payload.length());
  if (callback != nullptr) {
    callback();
    netManager.setColor();
  }
  server.send(200, F("text/plain"), F("OK"));
}

/**
 * Get data from the stream and send to the strip
 * @param payload stream data
 */
void NetManager::fromMqttStreamToStrip(char *payload) {
  uint32_t myLeds;
  char delimiters[] = ",";
  char *ptr;
  char *saveptr;
  char *ptrAtoi;

  uint16_t index = 0;
  ptr = strtok_r(payload, delimiters, &saveptr);
  uint16_t numLedFromLuciferin = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  uint8_t audioBrightness = strtoul(ptr, &ptrAtoi, 10);
  ptr = strtok_r(nullptr, delimiters, &saveptr);
  if (brightness != audioBrightness && !ldrEnabled) {
    brightness = audioBrightness;
  }
  if (numLedFromLuciferin == 0) {
    effect = Effect::solid;
  } else {
    if (ledManager.dynamicLedNum != numLedFromLuciferin) {
      LedManager::setNumLed(numLedFromLuciferin);
      ledManager.initLeds();
    }
    while (ptr != nullptr) {
      myLeds = strtoul(ptr, &ptrAtoi, 10);
      if (ldrInterval != 0 && ldrEnabled && ldrReading && ldrTurnOff) {
        ledManager.setPixelColor(index, 0, 0, 0);
      } else {
        ledManager.setPixelColor(index, (myLeds >> 16 & 0xFF), (myLeds >> 8 & 0xFF), (myLeds >> 0 & 0xFF));
      }
      index++;
      ptr = strtok_r(nullptr, delimiters, &saveptr);
    }
  }
  if (effect != Effect::solid) {
    framerateCounter++;
    lastStream = millis();
    ledManager.ledShow();
  }
}

/**
 * Process Firmware Configuration sent from Firefly Luciferin, this config requires reboot
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetManager::processFirmwareConfigWithReboot() {
  String deviceName = bootstrapManager.jsonDoc[F("deviceName")];
  String microcontrollerIP = bootstrapManager.jsonDoc[F("microcontrollerIP")];
  String mqttCheckbox = bootstrapManager.jsonDoc[F("mqttCheckbox")];
  String setSsid = bootstrapManager.jsonDoc[F("ssid")];
  String setEthd = bootstrapManager.jsonDoc[F("ethd")];

#if defined(ESP8266)
  setEthd = -1;
#endif
  String wifipwd = bootstrapManager.jsonDoc[F("wifipwd")];
  String mqttIP = bootstrapManager.jsonDoc[F("mqttIP")];
  String mqttPort = bootstrapManager.jsonDoc[F("mqttPort")];
  String mqttTopic = bootstrapManager.jsonDoc[F("mqttTopic")];
  String mqttuser = bootstrapManager.jsonDoc[F("mqttuser")];
  String mqttpass = bootstrapManager.jsonDoc[F("mqttpass")];
  String additionalParam = bootstrapManager.jsonDoc[F("additionalParam")];
  String gpioClockParam = bootstrapManager.jsonDoc[F("gpioClock")];
  String colorModeParam = bootstrapManager.jsonDoc[F("colorMode")];
  String colorOrderParam = bootstrapManager.jsonDoc[F("colorOrder")];
  String lednum = bootstrapManager.jsonDoc[F("lednum")];
  String br = bootstrapManager.jsonDoc[F("br")];
  JsonDocument doc;
  if (deviceName.length() > 0 &&
      ((mqttCheckbox == "false") || (mqttIP.length() > 0 && mqttPort.length() > 0 && mqttTopic.length() > 0))) {
    if (microcontrollerIP.length() == 0) {
      microcontrollerIP = "DHCP";
    }
    doc[F("ethd")] = bootstrapManager.jsonDoc[F("ethd")].isNull() ? String(ethd) : setEthd;

#if defined(ARDUINO_ARCH_ESP32)
    if (bootstrapManager.jsonDoc[F("ethd")].isNull() && ethd == 100) {
      doc[F("mosi")] = mosi;
      doc[F("miso")] = miso;
      doc[F("sclk")] = sclk;
      doc[F("cs")] = cs;
    } else if (!bootstrapManager.jsonDoc[F("ethd")].isNull()) {
      doc[F("mosi")] = bootstrapManager.jsonDoc[F("mosi")];
      doc[F("miso")] = bootstrapManager.jsonDoc[F("miso")];
      doc[F("sclk")] = bootstrapManager.jsonDoc[F("sclk")];
      doc[F("cs")] = bootstrapManager.jsonDoc[F("cs")];
    }
#endif

    doc[F("deviceName")] = deviceName;
    doc[F("microcontrollerIP")] = microcontrollerIP;
    doc[F("qsid")] = (setSsid != NULL && !setSsid.isEmpty()) ? setSsid : qsid;
    doc[F("qpass")] = (wifipwd != NULL && !wifipwd.isEmpty()) ? wifipwd : qpass;
    doc[F("OTApass")] = OTApass;
    if (mqttCheckbox.equals("true")) {
      doc[F("mqttIP")] = mqttIP;
      doc[F("mqttPort")] = mqttPort;
      doc[F("mqttTopic")] = mqttTopic;
      doc[F("mqttuser")] = mqttuser;
      doc[F("mqttpass")] = mqttpass;
    } else {
      doc[F("mqttIP")] = "";
      doc[F("mqttPort")] = "";
      doc[F("mqttTopic")] = "";
      doc[F("mqttuser")] = "";
      doc[F("mqttpass")] = "";
    }
    doc[F("additionalParam")] = !bootstrapManager.jsonDoc[F("additionalParam")].isNull()? additionalParam : String(gpioInUse);
    content = F("Success: rebooting the microcontroller using your credentials.");
    statusCode = 200;
  } else {
    content = F("Error: missing required fields.");
    statusCode = 404;
    Serial.println(F("Sending 404"));
  }
  delay(DELAY_500);
  server.sendHeader(F("Access-Control-Allow-Origin"), "*");
  server.send(statusCode, F("text/plain"), content);
  delay(DELAY_500);
  // Write to LittleFS
  Serial.println(F("Saving setup.json"));
  File jsonFile = LittleFS.open("/setup.json", FILE_WRITE);
  if (!jsonFile) {
    Serial.println(F("Failed to open [setup.json] file for writing"));
  } else {
    serializeJsonPretty(doc, Serial);
    serializeJson(doc, jsonFile);
    jsonFile.close();
    Serial.println(F("[setup.json] written correctly"));
  }
  if (!bootstrapManager.jsonDoc[F("lednum")].isNull()) {
    delay(DELAY_200);
    Serial.println(F("Saving lednum"));
    LedManager::setNumLed(lednum.toInt());
  }
  if (!bootstrapManager.jsonDoc[F("additionalParam")].isNull()) {
    delay(DELAY_200);
    Serial.println(F("Saving gpio"));
    Globals::setGpio(additionalParam.toInt());
  }
  if (!bootstrapManager.jsonDoc[F("gpioClock")].isNull()) {
    delay(DELAY_200);
    Serial.println(F("Saving gpio clock"));
    Globals::setGpioClock(gpioClockParam.toInt());
  }
  delay(DELAY_500);
  JsonDocument topicDoc;
  topicDoc[netManager.MQTT_PARAM] = mqttTopic;
  BootstrapManager::writeToLittleFS(topicDoc, netManager.TOPIC_FILENAME);
  if (!bootstrapManager.jsonDoc[F("colorMode")].isNull()) {
    delay(DELAY_500);
    ledManager.setColorMode(colorModeParam.toInt());
  }
  delay(DELAY_500);
  if (!bootstrapManager.jsonDoc[F("colorOrder")].isNull()) {
    ledManager.setColorOrder(colorOrderParam.toInt());
    delay(DELAY_500);
  }
  if (!bootstrapManager.jsonDoc[F("br")].isNull()) {
    Globals::setBaudRateInUse(br.toInt());
    Globals::setBaudRate(baudRateInUse);
  }
  delay(DELAY_1000);
#if defined(ARDUINO_ARCH_ESP32)
  if (ethd > 0 && ethd != -1) {
    EthManager::deallocateEthernetPins(ethd);
  }
  ESP.restart();
#elif defined(ESP8266)
  EspClass::restart();
#endif
  return true;
}

/**
 * Process Firmware Configuration sent from Firefly Luciferin
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetManager::processFirmwareConfig() {
  boolean espRestart = false;
  if (bootstrapManager.jsonDoc["MAC"].is<JsonVariant>()) {
    String macToUpdate = bootstrapManager.jsonDoc["MAC"];
    Serial.println(macToUpdate);
    if (macToUpdate == MAC) {
      // GPIO
      if (bootstrapManager.jsonDoc[GPIO_PARAM].is<JsonVariant>()) {
        int gpioFromConfig = (int) bootstrapManager.jsonDoc[GPIO_PARAM];
        if (gpioFromConfig != 0 && gpioInUse != gpioFromConfig) {
          Globals::setGpio(gpioFromConfig);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // GPIO CLOCK
      if (bootstrapManager.jsonDoc[GPIO_CLOCK_PARAM].is<JsonVariant>()) {
        int gpioClockFromConfig = (int) bootstrapManager.jsonDoc[GPIO_CLOCK_PARAM];
        if (gpioClockFromConfig != 0 && gpioClockInUse != gpioClockFromConfig) {
          Globals::setGpioClock(gpioClockFromConfig);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // COLOR_MODE
      if (bootstrapManager.jsonDoc[ledManager.COLOR_MODE_PARAM].is<JsonVariant>()) {
        int colorModeParam = (int) bootstrapManager.jsonDoc[ledManager.COLOR_MODE_PARAM];
        if (colorMode != colorModeParam) {
          colorMode = colorModeParam;
          ledManager.setColorMode(colorMode);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // COLOR_ORDER
      if (bootstrapManager.jsonDoc[ledManager.COLOR_ORDER_PARAM].is<JsonVariant>()) {
        int colorOrderParam = (int) bootstrapManager.jsonDoc[ledManager.COLOR_ORDER_PARAM];
        if (colorOrder != colorOrderParam) {
          colorOrder = colorOrderParam;
          ledManager.setColorOrder(colorOrder);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // LDR_PIN_PARAM
      if (bootstrapManager.jsonDoc[ledManager.LDR_PIN_PARAM].is<JsonVariant>()) {
        int ldrPinParam = (int) bootstrapManager.jsonDoc[ledManager.LDR_PIN_PARAM];
        if (ldrPin != ldrPinParam) {
          ldrPin = ldrPinParam;
          ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // RELAY_PIN_PARAM
      if (bootstrapManager.jsonDoc[ledManager.RELAY_PIN_PARAM].is<JsonVariant>()) {
        int relayPinParam = (int) bootstrapManager.jsonDoc[ledManager.RELAY_PIN_PARAM];
        if (relayPin != relayPinParam) {
          relayPin = relayPinParam;
          ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // INVERTED RELAY
      if (bootstrapManager.jsonDoc[ledManager.RELAY_INV_PARAM].is<JsonVariant>()) {
        bool relayInvParam =  bootstrapManager.jsonDoc[ledManager.RELAY_INV_PARAM];
        if (relInv != relayInvParam) {
          relInv = relayInvParam;
          ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // SB_PIN_PARAM
      if (bootstrapManager.jsonDoc[ledManager.SB_PIN_PARAM].is<JsonVariant>()) {
        int sbrPinParam = (int) bootstrapManager.jsonDoc[ledManager.SB_PIN_PARAM];
        if (sbPin != sbrPinParam) {
          sbPin = sbrPinParam;
          ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // BUILTIN LED
      if (bootstrapManager.jsonDoc[ledManager.LED_BUILTIN_PARAM].is<JsonVariant>()) {
        int ledBiParam = (int) bootstrapManager.jsonDoc[ledManager.LED_BUILTIN_PARAM];
        if (sbPin != ledBiParam) {
          ledBuiltin = ledBiParam;
          ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
          ledManager.reinitLEDTriggered = true;
        }
      }
      // Restart if needed
      if (ledManager.reinitLEDTriggered) {
        ledManager.reinitLEDTriggered = false;
        ledManager.initLeds();
      }
      if (espRestart) {
#if defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#elif defined(ESP8266)
        EspClass::restart();
#endif
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
bool NetManager::processUnSubscribeStream() {
  if (bootstrapManager.jsonDoc["instance"].is<JsonVariant>()) {
    String instance = bootstrapManager.jsonDoc["instance"];
    String manager = bootstrapManager.jsonDoc["manager"];
    if (manager.equals(deviceName)) {
      BootstrapManager::unsubscribe(netManager.streamTopic.c_str());
      netManager.streamTopic = netManager.baseStreamTopic + instance;
      effect = Effect::GlowWormWifi;
      Globals::turnOnRelay();
      ledManager.stateOn = true;
      BootstrapManager::subscribe(netManager.streamTopic.c_str(), 0);
    }
  }
  return true;
}

/**
 * Process JSON message
 * @param json StaticJsonDocument
 * @return true if message is correctly processed
 */
bool NetManager::processJson() {
  ledManager.lastLedUpdate = millis();
  boolean skipMacCheck = ((bootstrapManager.jsonDoc["MAC"].is<JsonVariant>() && bootstrapManager.jsonDoc["MAC"] == MAC)
                          || bootstrapManager.jsonDoc["allInstances"].is<JsonVariant>());
  if (!bootstrapManager.jsonDoc["MAC"].is<JsonVariant>() || skipMacCheck) {
    if (bootstrapManager.jsonDoc["state"].is<JsonVariant>()) {
      String state = bootstrapManager.jsonDoc["state"];
      if (state == ON_CMD) {
        Globals::turnOnRelay();
        ledManager.stateOn = true;
      } else if (state == OFF_CMD) {
        ledManager.stateOn = false;
      }
    }
    if (bootstrapManager.jsonDoc["color"].is<JsonVariant>()) {
      ledManager.red = bootstrapManager.jsonDoc["color"]["r"];
      ledManager.green = bootstrapManager.jsonDoc["color"]["g"];
      ledManager.blue = bootstrapManager.jsonDoc["color"]["b"];
    }
    if (bootstrapManager.jsonDoc["brightness"].is<JsonVariant>()) {
      brightness = bootstrapManager.jsonDoc["brightness"];
    }
    if (skipMacCheck) {
      if (bootstrapManager.jsonDoc["whitetemp"].is<JsonVariant>()) {
        uint8_t wt = bootstrapManager.jsonDoc["whitetemp"];
        if (wt != 0 && whiteTempInUse != wt) {
          LedManager::setWhiteTemp(wt);
        }
      }
    }
    if (bootstrapManager.jsonDoc["ffeffect"].is<JsonVariant>()) {
      ffeffect = bootstrapManager.jsonDoc["ffeffect"].as<String>();
    }
    String requestedEffect;
    if (bootstrapManager.jsonDoc["effect"].is<JsonVariant>()) {
      boolean effectIsDifferent = (effect != Effect::GlowWorm && effect != Effect::GlowWormWifi);
      requestedEffect = bootstrapManager.jsonDoc["effect"].as<String>();
      effect = Globals::stringToEffect(requestedEffect);
      if (effect == Effect::solid) {
        breakLoop = true;
      }
      if (skipMacCheck) {
        if (requestedEffect == "GlowWorm") {
          if (effectIsDifferent) {
            previousMillisLDR = 0;
          }
          effect = Effect::GlowWorm;
          ledManager.lastLedUpdate = millis();
        } else if (requestedEffect == "GlowWormWifi") {
          if (effectIsDifferent) {
            previousMillisLDR = 0;
          }
          effect = Effect::GlowWormWifi;
          lastStream = millis();
        }
      }
    }
    Effect reqEff = Globals::stringToEffect(requestedEffect);
    if ((autoSave && (ledManager.red != rStored || ledManager.green != gStored || ledManager.blue != bStored ||
                     brightness != brightnessStored || reqEff != effectStored || ledManager.stateOn != toggleStored))) {
      Globals::saveColorBrightnessInfo(ledManager.red, ledManager.green, ledManager.blue, brightness, requestedEffect, ledManager.stateOn);
    }
  }
  return true;
}

/**
 * Send microcontroller state
 * For debug: ESP.getFreeHeap() ESP.getHeapFragmentation() ESP.getMaxFreeBlockSize()
 */
void NetManager::sendStatus() {
  // Skip JSON framework for lighter processing during the stream
  if (effect == Effect::GlowWorm || effect == Effect::GlowWormWifi) {
    fpsData = F("{\"deviceName\":\"");
    fpsData += deviceName;
    fpsData += "\",\"color\": { \"r\": 255, \"g\": 190, \"b\": 140 }"; // Default for bias light
    fpsData += F(",\"state\":\"");
    fpsData += (ledManager.stateOn) ? ON_CMD : OFF_CMD;
    fpsData += F("\",\"brightness\":");
    fpsData += brightness;
    fpsData += F(",\"MAC\":\"");
    fpsData += MAC;
    fpsData += F("\",\"color_mode\":\"");
    fpsData += F("rgb");
    fpsData += F("\",\"lednum\":\"");
    fpsData += ledManager.dynamicLedNum;
    fpsData += F("\",\"framerate\":\"");
    fpsData += framerate > framerateSerial ? framerate : framerateSerial;
    fpsData += F("\",\"wifi\":\"");
    fpsData += BootstrapManager::getWifiQuality();
    if (ldrEnabled) {
      fpsData += F("\",\"ldr\":\"");
      fpsData += ((ldrValue * 100) / ldrDivider);
    }
    fpsData += F("\"}");
    if (mqttIP.length() > 0) {
      BootstrapManager::publish(netManager.lightStateTopic.c_str(), fpsData.c_str(), false);
    } else {
#if defined(ESP8266)
      if (netManager.remoteIpForUdp.isSet()) {
#elif defined(ARDUINO_ARCH_ESP32)
      if (!netManager.remoteIpForUdp.toString().equals(F("0.0.0.0"))) {
#endif
        netManager.broadcastUDP.beginPacket(netManager.remoteIpForUdp, UDP_BROADCAST_PORT);
        netManager.broadcastUDP.print(fpsData.c_str());
        netManager.broadcastUDP.endPacket();
      }
    }
  } else {
    bootstrapManager.jsonDoc.clear();
    JsonObject root = bootstrapManager.jsonDoc.to<JsonObject>();
    root[F("state")] = (ledManager.stateOn) ? ON_CMD : OFF_CMD;
    JsonObject color = root["color"].to<JsonObject>();
    color[F("r")] = ledManager.red;
    color[F("g")] = ledManager.green;
    color[F("b")] = ledManager.blue;
    color[F("colorMode")] = colorMode;
    color[F("colorOrder")] = colorOrder;
    root[F("color_mode")] = F("rgb");
    root[F("brightness")] = brightness;
    root[F("effect")] = Globals::effectToString(effect);
    root[F("deviceName")] = deviceName;
    root[F("IP")] = microcontrollerIP;
    root[F("dhcp")] = dhcpInUse;
    root[F("wifi")] = BootstrapManager::getWifiQuality();
    root[F("MAC")] = MAC;
    root[F("ver")] = VERSION;
    root[F("framerate")] = framerate > framerateSerial ? framerate : framerateSerial;
    if (ldrEnabled) {
      root[F("ldr")] = ((ldrValue * 100) / ldrDivider);
    }
    root[F("relayPin")] = relayPin;
    root[F("relayInv")] = relInv;
    root[F("sbPin")] = sbPin;
    root[F("ldrPin")] = ldrPin;
    root[F("ledBuiltin")] = ledBuiltin;
    root[BAUDRATE_PARAM] = baudRateInUse;
#if defined(ESP8266)
    root[F("board")] = F("ESP8266");
#endif
#if CONFIG_IDF_TARGET_ESP32C3
    root["board"] = "ESP32_C3_CDC";
#elif CONFIG_IDF_TARGET_ESP32S2
    root["board"] = "ESP32_S2";
#elif CONFIG_IDF_TARGET_ESP32C6
    root["board"] = "ESP32_C6";    
#elif CONFIG_IDF_TARGET_ESP32S3
#if ARDUINO_USB_MODE==1
    root["board"] = "ESP32_S3"; // CDC
#else
    root["board"] = "ESP32_S3";
#endif
#elif CONFIG_IDF_TARGET_ESP32
    root["board"] = "ESP32";
#endif
    root[LED_NUM_PARAM] = String(ledManager.dynamicLedNum);
    root[F("gpio")] = gpioInUse;
    root[F("gpioClock")] = gpioClockInUse;
    root[F("mqttopic")] = netManager.topicInUse;
    root[F("whitetemp")] = whiteTempInUse;
    if (effect == Effect::solid && !ledManager.stateOn) {
      LedManager::setColor(0, 0, 0);
    }

    // This topic should be retained, we don't want unknown values on battery voltage or wifi signal
    if (mqttIP.length() > 0) {
      BootstrapManager::publish(netManager.lightStateTopic.c_str(), root, true);
    } else {
      String output;
      serializeJson(root, output);
#if defined(ESP8266)
      if (netManager.remoteIpForUdpBroadcast.isSet()) {
#elif defined(ARDUINO_ARCH_ESP32)
      if (!netManager.remoteIpForUdpBroadcast.toString().equals(F("0.0.0.0"))) {
#endif
        netManager.broadcastUDP.beginPacket(netManager.remoteIpForUdpBroadcast, UDP_BROADCAST_PORT);
        netManager.broadcastUDP.print(output.c_str());
        netManager.broadcastUDP.endPacket();
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
bool NetManager::processMqttUpdate() {
  if (bootstrapManager.jsonDoc[F("update")].is<JsonVariant>()) {
    return processUpdate();
  }
  return true;
}

/**
 * Handle web server for the upgrade process
 * @return true if message is correctly processed
 */
bool NetManager::processUpdate() {
  Serial.println(F("Starting web server"));
  server.on(
    "/update",
    HTTP_POST,
    []() {
    },
    []() {
      HTTPUpload &upload = server.upload();
      updateSize = 480000;
#if defined(ARDUINO_ARCH_ESP32)
      esp_task_wdt_reset();
      updateSize = UPDATE_SIZE_UNKNOWN;
#endif
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update start: %s\n", upload.filename.c_str());
        if (!Update.begin(updateSize)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        server.sendHeader("Connection", "close");
        bool ok = Update.end(true);
        if (ok) {
          Serial.printf("Update success: %u bytes\n", upload.totalSize);
          server.send(200, "text/plain", "OK");
        } else {
          Update.printError(Serial);
          server.send(500, "text/plain", "KO");
        }
#if defined(ARDUINO_ARCH_ESP32)
        server.client().clear();
        esp_task_wdt_reset();
#endif
        if (ok) {
          if (mqttIP.length() > 0) {
            BootstrapManager::publish(
              netManager.updateResultStateTopic.c_str(),
              deviceName.c_str(),
              false
            );
          } else {
#if defined(ESP8266)
            if (netManager.remoteIpForUdp.isSet()) {
#elif defined(ARDUINO_ARCH_ESP32)
            if (!netManager.remoteIpForUdp.toString().equals(F("0.0.0.0"))) {
#endif
              netManager.broadcastUDP.beginPacket(netManager.remoteIpForUdp, UDP_BROADCAST_PORT);
              netManager.broadcastUDP.print(deviceName.c_str());
              netManager.broadcastUDP.endPacket();
            }
          }
        }
        delay(200);
#if defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#elif defined(ESP8266)
        EspClass::restart();
#endif
      }
    }
  );
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
bool NetManager::processGlowWormLuciferinRebootCmnd() {
  if (bootstrapManager.jsonDoc[VALUE] == OFF_CMD) {
    ledManager.stateOn = false;
    sendStatus();
    delay(1500);
#if defined(ARDUINO_ARCH_ESP32)
    ESP.restart();
#elif defined(ESP8266)
    EspClass::restart();
#endif
  }
  return true;
}

/**
 * Process LDR settings
 * @return true if message is correctly processed
 */
bool NetManager::processLDR() {
  if (bootstrapManager.jsonDoc[F("ldrEnabled")].is<JsonVariant>()) {
    stopUDP();
    String ldrEnabledMqtt = bootstrapManager.jsonDoc[F("ldrEnabled")];
    String ldrTurnOffMqtt = bootstrapManager.jsonDoc[F("ldrTurnOff")];
    String ldrIntervalMqtt = bootstrapManager.jsonDoc[F("ldrInterval")];
    String ldrMinMqtt = bootstrapManager.jsonDoc[F("ldrMin")];
    String ldrActionMqtt = bootstrapManager.jsonDoc[F("ldrAction")];
    String rPin = bootstrapManager.jsonDoc[F("relayPin")];
    String rInvStr = bootstrapManager.jsonDoc[F("relInv")];
    String sPin = bootstrapManager.jsonDoc[F("sbPin")];
    String lPin = bootstrapManager.jsonDoc[F("ldrPin")];
    String ledBi = bootstrapManager.jsonDoc[F("ledBuiltin")];
    relInv = rInvStr == "true";
    ldrEnabled = ldrEnabledMqtt == "true";
    ldrTurnOff = ldrTurnOffMqtt == "true";
    ldrInterval = ldrIntervalMqtt.toInt();
    ldrMin = ldrMinMqtt.toInt();
    if (ldrActionMqtt.toInt() == 2) {
      ldrDivider = ldrValue;
      ledManager.setLdr(ldrDivider);
      delay(DELAY_500);
    } else if (ldrActionMqtt.toInt() == 3) {
      ldrDivider = LDR_DIVIDER;
      ledManager.setLdr(-1);
      delay(DELAY_500);
    }
    ledManager.setLdr(ldrEnabledMqtt == "true", ldrTurnOffMqtt == "true",
                      ldrInterval, ldrMinMqtt.toInt());
    delay(DELAY_500);
    content = F("Success: rebooting the microcontroller using your credentials.");
    statusCode = 200;
    server.sendHeader(F("Access-Control-Allow-Origin"), "*");
    server.send(statusCode, F("text/plain"), content);
    delay(DELAY_500);
    if (!bootstrapManager.jsonDoc[F("relayPin")].isNull() && !bootstrapManager.jsonDoc[F("sbPin")].isNull() && !bootstrapManager.jsonDoc[F("ldrPin")].isNull()) {
      relayPin = rPin.toInt();
      sbPin = sPin.toInt();
      ldrPin = lPin.toInt();
      ledBuiltin = ledBi.toInt();
      ledManager.setPins(relayPin, sbPin, ldrPin, relInv, ledBuiltin);
    }
    delay(DELAY_500);
    startUDP();
  }
  return true;
}


/**
 * Execute the MQTT topic swap
 * @param customtopic new topic to use
 */
void NetManager::executeMqttSwap(const String &customtopic) {
  Serial.println("Swapping topic=" + customtopic);
  netManager.topicInUse = customtopic;
  swapTopicUnsubscribe();
  swapTopicReplace(customtopic);
  swapTopicSubscribe();
}

#endif

/**
 * Check connection and turn off the LED strip if no input received
 */
void NetManager::checkConnection() {
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);
  server.handleClient();
  currentMillisCheckConn = millis();
  if (currentMillisCheckConn - prevMillisCheckConn1 > 1000) {
    prevMillisCheckConn1 = currentMillisCheckConn;
    // No updates since 7 seconds, turn off LEDs
    if ((!breakLoop && (effect == Effect::GlowWorm) && (currentMillisCheckConn > ledManager.lastLedUpdate + 10000)) ||
        (!breakLoop && (effect == Effect::GlowWormWifi) && (currentMillisCheckConn > lastStream + 10000))) {
      breakLoop = true;
      effect = Effect::solid;
      ledManager.stateOn = false;
      Globals::turnOffRelay();
    }
    framerate = framerateCounter > 0 ? framerateCounter / 1 : 0;
    framerateCounter = 0;
    NetManager::sendStatus();
  }
#elif  TARGET_GLOWWORMLUCIFERINLIGHT
  currentMillisCheckConn = millis();
  if (currentMillisCheckConn - prevMillisCheckConn2 > 15000) {
    prevMillisCheckConn2 = currentMillisCheckConn;
    // No updates since 15 seconds, turn off LEDs
    if(currentMillisCheckConn > ledManager.lastLedUpdate + 10000){
      LedManager::setColor(0, 0, 0);
      globals.turnOffRelay();
    }
  }
#endif
  Globals::sendSerialInfo();
}