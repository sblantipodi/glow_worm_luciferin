/*
  NetManager.h - Glow Worm Luciferin for Firefly Luciferin
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

#ifndef GLOW_WORM_LUCIFERIN_NET_MANAGER_H
#define GLOW_WORM_LUCIFERIN_NET_MANAGER_H

#include <Arduino.h>
#include "Version.h"
#include "WebSettings.h"
#include "Globals.h"

const uint8_t UDP_CHUNK_SIZE = 140; // this value must match with the one in Firefly Luciferin
const uint16_t UDP_MAX_BUFFER_SIZE = (UDP_CHUNK_SIZE * 10) + 51; // this value must match with the one in Firefly Luciferin
const uint8_t UDP_BR_MAX_BUFFER_SIZE = 50;
// Safety cap: drain a bounded number of queued UDP packets per loop() iteration.
// This prevents UDP bursts from starving web server, button, LDR, and watchdog tasks.
// This is not a fixed delay: when the UDP queue is almost empty, the loop exits immediately.
// The value matters only when packets are piling up faster than the firmware can display them.
constexpr const uint8_t UDP_MAX_PACKETS_PER_LOOP = 12;

struct RleEntry {
    uint8_t count;
    uint8_t size;
};

// RLE/stream state (udpFrameReady, rle, numRleEntries, rleTableValid,
// rleCumCount, rleCumPhys, cachedRleTotalPhys, cachedRleFrameNum,
// currentFrameValid, lastProcessedChunkNum, lastChunkFrame) is defined in NetManager.cpp
// Keeping the definitions out of this header avoids a 500+ byte BSS copy in every TU.

// RLE helpers shared by the UDP and serial paths. The group map (rle[]) expands
// virtual colors onto the strip: entry {count, size} = "the next `count` colors
// each use `size` consecutive physical LEDs". rleBuildCumTables() rebuilds the
// prefix sums at every site that writes rle[], so per-color lookups are O(log)
// instead of an O(entries) scan:
//   cumCount[i] = colors of entries [0..i-1]   (cumCount[0] = 0)
//   cumPhys[i]  = physical LEDs of entries [0..i-1] (cumPhys[0] = 0)
// so entry i spans colors [cumCount[i], cumCount[i+1]) and LEDs [cumPhys[i],
// cumPhys[i+1)]. Both fit in uint16_t: cumCount <= 250 x 255 = 63750 < 65536,
// and cumPhys holds the strip's physical LED count (< 65536 in practice).
inline void rleBuildCumTables(const RleEntry *table, uint8_t numEntries, uint16_t *cumCount, uint16_t *cumPhys) {
    cumCount[0] = 0;
    cumPhys[0] = 0;
    for (uint8_t i = 0; i < numEntries; i++) {
        cumCount[i + 1] = (uint16_t)(cumCount[i] + table[i].count);
        cumPhys[i + 1] = (uint16_t)(cumPhys[i] + (uint16_t)table[i].count * table[i].size);
    }
}

// Binary search on the non-decreasing cumCount: first entry i with
// cumCount[i] <= index < cumCount[i+1], i.e. the entry covering color index
// (numEntries if index is past the last color; at most 8 steps for 250 entries).
// Zero-count entries never match: their color range is empty.
inline uint8_t rleFindEntry(const uint16_t *cumCount, uint8_t numEntries, uint16_t index) {
    uint8_t lo = 0;
    uint8_t hi = numEntries;
    while (lo < hi) {
        uint8_t mid = (uint8_t)((lo + hi) / 2);
        if (cumCount[mid + 1] <= index) {
            lo = (uint8_t)(mid + 1);
        } else {
            hi = mid;
        }
    }
    return lo;
}

// Physical size of the group holding color "index" (1 when index is out of range).
inline uint8_t rleGetGroupSize(const RleEntry *table, const uint16_t *cumCount, uint8_t numEntries, uint16_t index) {
    uint8_t i = rleFindEntry(cumCount, numEntries, index);
    if (i >= numEntries) return 1;
    return table[i].size;
}

// Physical LED offset of color "colorIndex" (totalPhys when colorIndex is out of range).
inline uint16_t rleComputePhysOffset(const RleEntry *table, const uint16_t *cumCount, const uint16_t *cumPhys, uint8_t numEntries, uint16_t colorIndex) {
    uint8_t i = rleFindEntry(cumCount, numEntries, colorIndex);
    if (i >= numEntries) return cumPhys[numEntries];
    return (uint16_t)(cumPhys[i] + (uint16_t)(colorIndex - cumCount[i]) * table[i].size);
}

// Total physical LEDs of the table (O(1) now that cumPhys is precomputed).
inline uint16_t rleTotalPhys(const uint16_t *cumPhys, uint8_t numEntries) {
    return cumPhys[numEntries];
}

class NetManager {

public:

    WiFiUDP UDP;
    WiFiUDP broadcastUDP;

    #define UDP_PORT 4210 // this value must match with the one in Firefly Luciferin
    #define UDP_BROADCAST_PORT 5001 // this value must match with the one in Firefly Luciferin
    char packet[UDP_MAX_BUFFER_SIZE];
    char packetBroadcast[UDP_BR_MAX_BUFFER_SIZE];
    char broadCastAddress[UDP_BR_MAX_BUFFER_SIZE];
    char dname[UDP_BR_MAX_BUFFER_SIZE];

    String lightStateTopic = "lights/glowwormluciferin";
    String updateStateTopic = "lights/glowwormluciferin/update";
    String helloTopic = "stat/glowwormluciferin/hello";
    String updateResultStateTopic = "lights/glowwormluciferin/update/result";
    String lightSetTopic = "lights/glowwormluciferin/set";
    String effectToGw = "lights/glowwormluciferin/effectToGw";
    String effectToFw = "lights/glowwormluciferin/effectToFf";
    String baseStreamTopic = "lights/glowwormluciferin/set/stream";
    String streamTopic = "lights/glowwormluciferin/set/stream";
    String unsubscribeTopic = "lights/glowwormluciferin/unsubscribe";
    String cmndReboot = "cmnd/glowwormluciferin/reboot";
    String firmwareConfigTopic = "lights/glowwormluciferin/firmwareconfig";
    String deviceTopic = "lights/glowwormluciferin/device";
    const char *BASE_TOPIC = "glowwormluciferin";
    String topicInUse = "glowwormluciferin";
    static constexpr const char *MQTT_PARAM = "mqttopic";
    static constexpr const char *TOPIC_FILENAME = "topic.json";
    bool servingWebPages = false;

    IPAddress remoteIpForUdp;
    IPAddress remoteIpForUdpBroadcast;

    [[maybe_unused]] static boolean firmwareUpgrade;
    static size_t updateSize;
    String prefsData; // save space on default constructor
    char STOP_FF[50] = "{\"state\":\"ON\",\"startStopInstances\":\"STOP\"}";

    void getUDPStream();

    static void fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]);

    static void fromMqttStreamToStrip(char *payload);

    static void httpCallback(bool (*callback)());

    void listenOnHttpGet();

    static void startUDP();

    static void stopUDP();

    static void swapTopicUnsubscribe();

    static void swapTopicReplace(const String &customtopic);

    static void swapTopicSubscribe();

    static bool processUpdate();

    static bool processMqttUpdate();

    static bool processJson();

    static bool processFirmwareConfig();

    static bool processFirmwareConfigWithReboot();

    static bool processGlowWormLuciferinRebootCmnd();

    static bool processSetIp();

    static bool processLDR();

    static bool processUnSubscribeStream();

    static void manageDisconnections();

    static void manageQueueSubscription();

    static void executeMqttSwap(const String &customtopic);

    static void callback(char *topic, byte *payload, unsigned int length);

    static void manageHardwareButton();

    static void sendStatus();

    static void checkConnection();

    static void setLeds();

    static void setColor();

    void manageAPSetting(bool isSettingRoot);

    static void parseRleGroupMap(char* saveptr);

};

#endif //GLOW_WORM_LUCIFERIN_NET_MANAGER_H
