/*
  EffectsManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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
      ledManager.setNumLed(numLedFromLuciferin);
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
