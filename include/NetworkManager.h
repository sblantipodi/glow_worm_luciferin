/*
  NetworkManager.h - Glow Worm Luciferin for Firefly Luciferin
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

#ifndef GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H
#define GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H

#include <Arduino.h>
#include "Version.h"
#include "WebSettings.h"
#include "Globals.h"

const uint8_t UDP_CHUNK_SIZE = 140; // this value must match with the one in Firefly Luciferin
const uint16_t UDP_MAX_BUFFER_SIZE = 4096; // this value must match with the one in Firefly Luciferin

class NetworkManager {

  public:

    WiFiUDP UDP;
    WiFiUDP broadcastUDP;

    #define UDP_PORT 4210 // this value must match with the one in Firefly Luciferin
    #define UDP_BROADCAST_PORT 5001 // this value must match with the one in Firefly Luciferin
    char packet[UDP_MAX_BUFFER_SIZE];
    char packetBroadcast[UDP_MAX_BUFFER_SIZE];
    static const uint16_t FIRST_CHUNK = 170;
    static const uint16_t SECOND_CHUNK = 340;
    static const uint16_t THIRD_CHUNK = 510;
    static uint16_t part;

    String lightStateTopic = "lights/glowwormluciferin";
    String updateStateTopic = "lights/glowwormluciferin/update";
    String updateResultStateTopic = "lights/glowwormluciferin/update/result";
    String lightSetTopic = "lights/glowwormluciferin/set";
    String baseStreamTopic = "lights/glowwormluciferin/set/stream";
    String streamTopic = "lights/glowwormluciferin/set/stream";
    String unsubscribeTopic = "lights/glowwormluciferin/unsubscribe";
    String cmndReboot = "cmnd/glowwormluciferin/reboot";
    String fpsTopic = "lights/glowwormluciferin/fps";
    String firmwareConfigTopic = "lights/glowwormluciferin/firmwareconfig";
    String deviceTopic = "lights/glowwormluciferin/device";
    const char *BASE_TOPIC = "glowwormluciferin";
    String topicInUse = "glowwormluciferin";
    const String MQTT_PARAM = "mqttopic";
    const String TOPIC_FILENAME = "topic.json";
    bool JSON_STREAM = false; // DEPRECATED
    bool servingWebPages = false;

    IPAddress remoteBroadcastPort;

    [[maybe_unused]] static boolean firmwareUpgrade;
    static size_t updateSize;
    static String fpsData; // save space on default constructor
    String prefsData; // save space on default constructor
    char START_FF[50] = "{\"state\":\"ON\",\"startStopInstances\":\"PLAY\"}";
    char STOP_FF[50] = "{\"state\":\"ON\",\"startStopInstances\":\"STOP\"}";

    void getUDPStream();
    static void fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]);
    static void fromMqttStreamToStrip(char *payload);
    static void httpCallback(bool (*callback)());
    void listenOnHttpGet();
    static void startUDP();
    static void stopUDP();
    static void swapTopicUnsubscribe();
    static void swapTopicReplace(const String& customtopic);
    static void swapTopicSubscribe();
    static bool processUpdate();
    static bool processMqttUpdate();
    static bool processJson();
    static bool processFirmwareConfig();
    static bool processGlowWormLuciferinRebootCmnd();
    static bool processLDR();
    static bool processUnSubscribeStream();
    bool swapMqttTopic();
    static void jsonStream(byte *payload, unsigned int length);
    static void manageDisconnections();
    static void manageQueueSubscription();
    static void executeMqttSwap(const String& customtopic);
    static void callback(char* topic, byte* payload, unsigned int length);
    static void manageHardwareButton();
    static void sendStatus();
    static void checkConnection();
    void setLeds() const;
};

#endif //GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H
