/*
  NetworkManager.h - Glow Worm Luciferin for Firefly Luciferin
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

#ifndef GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H
#define GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H

#include <Arduino.h>
#include "Globals.h"

const uint8_t UDP_CHUNK_SIZE = 140; // this value must match with the one in Firefly Luciferin
const uint16_t UDP_MAX_BUFFER_SIZE = 4096; // this value must match with the one in Firefly Luciferin

class NetworkManager {

private:


public:

    WiFiUDP UDP;
    WiFiUDP broadcastUDP;

    #define UDP_PORT 4210 // this value must match with the one in Firefly Luciferin
    #define UDP_BROADCAST_PORT 5001 // this value must match with the one in Firefly Luciferin

    String prefsTopic = "/prefs";
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
    const char *GET_SETTINGS = "/getsettings";
    String topicInUse = "glowwormluciferin";
    bool JSON_STREAM = false; // DEPRECATED
    bool servingWebPages = false;

    IPAddress remoteBroadcastPort;
    char packet[UDP_MAX_BUFFER_SIZE];
    char packetBroadcast[UDP_MAX_BUFFER_SIZE];

    void getUDPStream();
    void fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]);



};

#endif //GLOW_WORM_LUCIFERIN_NETWORK_MANAGER_H
