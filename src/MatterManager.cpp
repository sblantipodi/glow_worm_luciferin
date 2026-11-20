/*
  MatterManager.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#ifdef GLOW_WORM_MATTER

#include <FS.h>
#include <Arduino.h>
#include <Matter.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>
#include <platform/PlatformManager.h>
#include <setup_payload/SetupPayload.h>

#include "Globals.h"
#include "NetManager.h"
#include "MatterManager.h"

namespace {

MatterEnhancedColorLight light;
constexpr const char *pairingPreferences = "gw-matter";
constexpr const char *passcodeKey = "passcode";

enum class PairingReset { idle, scheduled, restarting };
PairingReset pairingReset = PairingReset::idle;
unsigned long resetRequestedAt = 0;

struct LightState {
  bool on;
  uint8_t level;
  espRgbColor_t rgb;
};

// Matter callbacks run on the CHIP task. The Arduino loop consumes these fields
// under the CHIP stack lock and applies the command after releasing the lock.
struct LightChanges {
  bool power = false;
  bool level = false;
  bool color = false;
  LightState state{};

  bool any() const { return power || level || color; }
};

LightChanges pending;
LightState reported{};
bool publishing = false;

LightState readLight() {
  return {ledManager.stateOn, brightness, {ledManager.red, ledManager.green, ledManager.blue}};
}

bool sameColor(espRgbColor_t first, espRgbColor_t second) {
  return first.r == second.r && first.g == second.g && first.b == second.b;
}

uint8_t matterLevel(uint8_t level) {
  return max(1, (level * 254 + 127) / 255);
}

espHsvColor_t matterColor(const LightState &state) {
  espHsvColor_t hsv = espRgbColorToHsvColor(state.rgb);
  hsv.h = min(static_cast<uint8_t>(hsv.h), uint8_t{254});
  hsv.s = (hsv.s * 254 + 127) / 255;
  hsv.v = matterLevel(state.level);
  return hsv;
}

uint32_t newPasscode(uint32_t previous) {
  uint32_t passcode;
  do {
    passcode = 1 + esp_random() % 99999998;
  } while (passcode == previous || !chip::SetupPayload::IsValidSetupPIN(passcode));
  return passcode;
}

bool configurePairing() {
  Preferences preferences;
  if (!preferences.begin(pairingPreferences)) {
    return false;
  }
  uint32_t passcode = preferences.getUInt(passcodeKey, 0);
  uint16_t discriminator = preferences.getUShort("discriminator", 0xffff);
  if (!chip::SetupPayload::IsValidSetupPIN(passcode)) {
    passcode = newPasscode(passcode);
    if (preferences.putUInt(passcodeKey, passcode) != sizeof(passcode)) {
      return false;
    }
  }
  if (discriminator > 0x0fff) {
    discriminator = esp_random() & 0x0fff;
    if (preferences.putUShort("discriminator", discriminator) != sizeof(discriminator)) {
      return false;
    }
  }
  return Matter.setSetupPasscode(passcode) && Matter.setSetupDiscriminator(discriminator);
}

bool authorizePairingReset() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  if (OTApass.isEmpty() || OTApass == F("XXX")) {
    return true;
  }
  if (!server.authenticate([](HTTPAuthMethod mode, String username, String[]) -> String * {
    return mode == DIGEST_AUTH && username.equalsConstantTime(F("admin")) ? new String(OTApass) : nullptr;
  })) {
    server.requestAuthentication(DIGEST_AUTH, "Glow Worm Matter");
    return false;
  }
  return true;
}

void sendPairingCode() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  if (pairingReset != PairingReset::idle) {
    server.send(503, F("application/json"), F("{\"status\":\"resetting\"}"));
    return;
  }
  auto &platform = chip::DeviceLayer::PlatformMgr();
  if (!Matter.isStackStarted() || !platform.TryLockChipStack()) {
    server.send(503, F("application/json"), F("{\"status\":\"unavailable\"}"));
    return;
  }
  const bool paired = Matter.isDeviceCommissioned();
  const String code = paired ? String() : Matter.getManualPairingCode();
  platform.UnlockChipStack();

  if (paired) {
    server.send(200, F("application/json"), F("{\"status\":\"paired\"}"));
  } else if (code.isEmpty()) {
    server.send(503, F("application/json"), F("{\"status\":\"unavailable\"}"));
  } else {
    JsonDocument response;
    response["status"] = "ready";
    response["pairingCode"] = code;
    String payload;
    serializeJson(response, payload);
    server.send(200, F("application/json"), payload);
  }
}

void requestPairingReset() {
  if (!authorizePairingReset()) {
    return;
  }
  // Requiring a JSON POST prevents a cross-origin form from using cached credentials.
  String mediaType = server.header(F("Content-Type"));
  const int parameters = mediaType.indexOf(';');
  if (parameters >= 0) {
    mediaType.remove(parameters);
  }
  mediaType.trim();
  if (!mediaType.equalsIgnoreCase(F("application/json"))) {
    server.send(415, F("application/json"), F("{\"error\":\"Send application/json.\"}"));
    return;
  }
  JsonDocument command;
  if (deserializeJson(command, server.arg(F("plain"))) || !command["reset"].is<bool>() ||
      !command["reset"].as<bool>()) {
    server.send(400, F("application/json"), F("{\"error\":\"Confirm reset with reset: true.\"}"));
    return;
  }
  if (!Matter.isStackStarted()) {
    server.send(503, F("application/json"), F("{\"error\":\"Matter is not ready.\"}"));
    return;
  }
  if (pairingReset == PairingReset::idle) {
    Preferences preferences;
    if (!preferences.begin(pairingPreferences) ||
        preferences.putUInt(passcodeKey, newPasscode(preferences.getUInt(passcodeKey, 0))) != sizeof(uint32_t)) {
      server.send(500, F("application/json"), F("{\"error\":\"Could not save a new Matter passcode.\"}"));
      return;
    }
    resetRequestedAt = millis();
    pairingReset = PairingReset::scheduled;
  }
  server.send(202, F("application/json"), F("{\"status\":\"resetting\"}"));
}

bool startMatter() {
  // Keep BLE commissioning enabled for discovery by phone commissioners.
  // Arduino Matter releases its BLE memory after successful commissioning.
  if (!Matter.selectNetwork(MATTER_NETWORK_WIFI) || !configurePairing()) {
    return false;
  }
  Matter.setVendorName(AUTHOR);
  Matter.setProductName("Glow Worm Luciferin");
  Matter.setDeviceName("Glow Worm Luciferin");
  Matter.setSoftwareVersionString(VERSION);

  reported = readLight();
  if (!light.begin(reported.on, matterColor(reported), matterLevel(reported.level),
                   DAYLIGHT_WHITE_COLOR_TEMPERATURE.ctMireds)) {
    return false;
  }
  Matter.begin();
  if (!Matter.isStackStarted()) {
    return false;
  }

  esp_matter::lock::ScopedChipStackLock lock(portMAX_DELAY);
  light.onChangeOnOff([](bool on) {
    if (!publishing) {
      pending.power = true;
      pending.state.on = on;
    }
    return true;
  });
  light.onChangeBrightness([](uint8_t level) {
    if (!publishing) {
      pending.level = true;
      pending.state.level = (level * 255 + 127) / 254;
    }
    return true;
  });
  light.onChangeColorHSV([](espHsvColor_t hsv) {
    if (!publishing) {
      pending.color = true;
      const uint8_t saturation = (min(hsv.s, uint8_t{254}) * 255 + 127) / 254;
      pending.state.rgb = espHsvToRgbColor(hsv.h, saturation, 255);
    }
    return true;
  });
  light.onChangeColorTemperature([](uint16_t mireds) {
    if (!publishing && mireds != 0) {
      pending.color = true;
      pending.state.rgb = espCTToRgbColor(mireds);
    }
    return true;
  });
  return true;
}

void publishLight(const LightState &state) {
  publishing = true;
  if (!sameColor(state.rgb, reported.rgb)) {
    light.setColorHSV(matterColor(state));
  }
  if (state.level != reported.level) {
    light.setBrightness(matterLevel(state.level));
  }
  if (state.on != reported.on) {
    light.setOnOff(state.on);
  }
  publishing = false;
  reported = state;
}

void applyChanges(const LightChanges &changes, bool streaming) {
  JsonDocument command;
  if (changes.power) {
    reported.on = changes.state.on;
    if (reported.on != ledManager.stateOn || (!reported.on && streaming)) {
      command["state"] = reported.on ? ON_CMD : OFF_CMD;
    }
  }
  if (changes.level) {
    reported.level = changes.state.level;
    if (reported.level != brightness) {
      command["brightness"] = reported.level;
    }
  }
  if (changes.color) {
    reported.rgb = changes.state.rgb;
    if (!sameColor(reported.rgb, readLight().rgb)) {
      JsonObject rgb = command["color"].to<JsonObject>();
      rgb["r"] = reported.rgb.r;
      rgb["g"] = reported.rgb.g;
      rgb["b"] = reported.rgb.b;
    }
  }
  if (!command.isNull()) {
    command["effect"] = "Solid";
    NetManager::setLeds(command);
    NetManager::setColor();
  }
}

} // namespace

void MatterManager::registerHttpHandlers() {
  const char *headers[] = {"Content-Type"};
  server.collectHeaders(headers, 1);
  server.on(F("/matter/pairing"), HTTP_GET, sendPairingCode);
  server.on(F("/matter/reset"), HTTP_POST, requestPairingReset);
}

void MatterManager::loop() {
  if (pairingReset != PairingReset::idle) {
    if (pairingReset == PairingReset::scheduled && millis() - resetRequestedAt >= DELAY_200) {
      pairingReset = PairingReset::restarting;
      Helpers::prepareRestart();
      Matter.decommission();
    }
    return;
  }
  static bool ipv6Requested = false;
  static bool startAttempted = false;
  static bool started = false;
  if (!startAttempted) {
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
    if (!ipv6Requested) {
      ipv6Requested = WiFi.STA.enableIPv6();
    }
    if (!WiFi.STA.hasLinkLocalIPv6()) {
      return;
    }
    startAttempted = true;
    started = startMatter();
    if (!started) {
      Serial.println(F("Matter over Wi-Fi could not start"));
    }
  }
  if (!started) {
    return;
  }

  static unsigned long lastSync = 0;
  static bool pairingCodePrinted = false;
  const unsigned long now = millis();
  if (now - lastSync < 100) {
    return;
  }
  auto &platform = chip::DeviceLayer::PlatformMgr();
  if (!platform.TryLockChipStack()) {
    return;
  }
  lastSync = now;
  const LightChanges changes = pending;
  pending = {};
  const bool streaming = effect == Effect::GlowWorm || effect == Effect::GlowWormWifi;
  // Streaming can change brightness on every packet. Track the light's
  // configured brightness separately from those per-frame changes.
  if (!changes.any()) {
    LightState state = readLight();
    if (streaming) {
      state.level = reported.level;
    }
    publishLight(state);
  }
  String pairingCode;
  if (!Serial || Matter.isDeviceCommissioned()) {
    pairingCodePrinted = false;
  } else if (!pairingCodePrinted) {
    pairingCode = Matter.getManualPairingCode();
    pairingCodePrinted = !pairingCode.isEmpty();
  }
  platform.UnlockChipStack();

  if (changes.any()) {
    applyChanges(changes, streaming);
  }
  if (!pairingCode.isEmpty()) {
    Serial.print(F("Matter pairing code: "));
    Serial.println(pairingCode);
  }
}

#endif
