/*
  GlowWormLuciferin.h - Glow Worm Luciferin for Firefly Luciferin
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
*/
#if defined(ESP32)
//#define FASTLED_INTERRUPT_RETRY_COUNT 0
//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP32_I2S true
#endif
#include <FastLED.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "Version.h"
#include "BootstrapManager.h"
#include "EffectsManager.h"
#if defined(ESP32)
#include <soc/timer_group_struct.h>
#include <soc/timer_group_reg.h>
#elif defined(ESP8266)
#include "PingESP.h"
#endif


/****************** BOOTSTRAP MANAGER ******************/
BootstrapManager bootstrapManager;
EffectsManager effectsManager;
Helpers helper;
#if defined(ESP32)
TaskHandle_t handleTcpTask = NULL; // fast TCP task pinned to CORE0
TaskHandle_t handleSerialTask = NULL; // fast Serial task pinned to CORE1
#define RELAY_PIN 23
#elif defined(ESP8266)
#define RELAY_PIN 12
PingESP pingESP;
#endif


/************* MQTT TOPICS (change these topics as you wish)  **************************/
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
const char* BASE_TOPIC = "glowwormluciferin";
String topicInUse = "glowwormluciferin";
bool JSON_STREAM = false; // DEPRECATED
boolean reinitLEDTriggered = false;
uint8_t whiteTempCorrection[] = {255, 255, 255};

enum class Effect { GlowWormWifi, GlowWorm, solid, fire, twinkle, bpm, rainbow, chase_rainbow, solid_rainbow, mixed_rainbow };
Effect effect;

/****************** Glow Worm Luciferin ******************/
// DPsoftware Checksum
uint8_t prefix[] = {'D', 'P', 's', 'o', 'f', 't'}, hi, lo, chk, loSecondPart, usbBrightness, gpio, baudRate, whiteTemp, fireflyEffect, i;
bool led_state = true;
uint lastLedUpdate = 10000;
uint lastStream = 0;
float framerate = 0;
float framerateCounter = 0;
uint8_t gpioInUse = 2, baudRateInUse = 3, fireflyEffectInUse, whiteTempInUse;
// Upgrade firmware
boolean firmwareUpgrade = false;
size_t updateSize = 0;
String fpsData((char*)0); // save space on default constructor

/****************** FastLED Defintions ******************/
#define NUM_LEDS 511 // Max Led support
CRGB leds[NUM_LEDS];
uint16_t dynamicLedNum = NUM_LEDS;
const String LED_NUM_FILENAME = "led_number.json";
const String GPIO_FILENAME = "gpio.json";
const String TOPIC_FILENAME = "topic.json";
const String BAUDRATE_FILENAME = "baudrate.json";
const String EFFECT_FILENAME = "effect.json";
const String LED_NUM_PARAM = "lednum";
const String GPIO_PARAM = "gpio";
const String MQTT_PARAM = "mqttopic";
const String BAUDRATE_PARAM = "baudrate";
const String EFFECT_PARAM = "effect";
#define UDP_PORT 4210 // this value must match with the one in Firefly Luciferin
#define UDP_BROADCAST_PORT 5001 // this value must match with the one in Firefly Luciferin
WiFiUDP UDP;
WiFiUDP broadcastUDP;
const uint8_t UDP_CHUNK_SIZE = 140; // this value must match with the one in Firefly Luciferin
const uint16_t UDP_MAX_BUFFER_SIZE = 4096; // this value must match with the one in Firefly Luciferin
IPAddress remoteBroadcastPort;

const uint16_t FIRST_CHUNK = 170;
const uint16_t SECOND_CHUNK = 340;
const uint16_t THIRD_CHUNK = 510;
#define DATA_PIN    5 // Wemos D1 Mini Lite PIN D5
//#define CLOCK_PIN 5
#define CHIPSET     WS2812B
#define COLOR_ORDER GRB
#define UDP_BROAD

byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

/****************** GLOBALS for fade/flash ******************/
bool stateOn = false;
bool relayState = false;

//NOISE
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
CRGBPalette16 targetPalette(OceanColors_p);
CRGBPalette16 currentPalette(CRGB::Black);

bool breakLoop = false;
uint16_t part = 1;

/****************** FUNCTION DECLARATION (NEEDED BY PLATFORMIO WHILE COMPILING CPP FILES) ******************/
// Bootstrap functions
void callback(char* topic, byte* payload, unsigned int length);
void manageDisconnections();
void manageQueueSubscription();
void manageHardwareButton();
// Project specific functions
void sendStatus();
bool processUpdate();
bool processJson();
bool processFirmwareConfig();
bool processGlowWormLuciferinRebootCmnd();
bool processUnSubscribeStream();
bool swapMqttTopic();
void executeMqttSwap(String customtopic);
void setColor(uint8_t inR, uint8_t inG, uint8_t inB);
void checkConnection();
void tcpTask(void * parameter);
void serialTask(void * parameter);
void mainLoop();
void sendSerialInfo();
void feedTheDog();
void setGpio(int gpio);
void setBaudRate(int baudRate);
void setNumLed(int numLedFromLuciferin);
int setBaudRateInUse(int baudRate);
void swapTopicUnsubscribe();
void swapTopicReplace(String customtopic);
void swapTopicSubscribe();
void setTemperature(int whitetemp);
void jsonStream(byte *payload, unsigned int length);
void turnOffRelay();
void turnOnRelay();
uint8_t applyWhiteTempRed(uint8_t r);
uint8_t applyWhiteTempGreen(uint8_t g);
uint8_t applyWhiteTempBlue(uint8_t b);
uint8_t applyBrightnessCorrection(uint8_t c);
void setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void ledShow();
void initLeds();
void fromUDPStreamToStrip(char (&payload)[UDP_MAX_BUFFER_SIZE]);
void fromMqttStreamToStrip(char *payload);
void cleanLEDs();
void getUDPStream();
void httpCallback(bool (*callback)());
void listenOnHttpGet();
