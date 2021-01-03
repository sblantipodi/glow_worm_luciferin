/*
  GlowWormLuciferin.h - Glow Worm Luciferin for Firefly Luciferin
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

#include <FastLED.h>
#include "Version.h"
#include "BootstrapManager.h"
#if defined(ESP32)
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#endif

/****************** BOOTSTRAP MANAGER ******************/
BootstrapManager bootstrapManager;
Helpers helper;

/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char* LIGHT_STATE_TOPIC = "lights/glowwormluciferin";
const char* UPDATE_STATE_TOPIC = "lights/glowwormluciferin/update";
const char* UPDATE_RESULT_STATE_TOPIC = "lights/glowwormluciferin/update/result";
const char* LIGHT_SET_TOPIC = "lights/glowwormluciferin/set";
const char* STREAM_TOPIC = "lights/glowwormluciferin/set/stream";
const char* TIME_TOPIC = "stat/time";
const char* CMND_AMBI_REBOOT = "cmnd/glowwormluciferin/reboot";
const char* FPS_TOPIC = "lights/glowwormluciferin/fps";

boolean statusSent = false;

enum class Effect { solid, GlowWorm, GlowWormWifi, bpm, candy_cane, confetti, cyclon_rainbow, dots,
        fire, glitter, juggle, lightning, police_all, police_one, rainbow, solid_rainbow, rainbow_with_glitter,
        sinelon, twinkle, noise, ripple };
Effect effect;

/****************** Glow Worm Luciferin ******************/
#define max_bright 255       // maximum brightness (0 - 255)
#define min_bright 50        // the minimum brightness (0 - 255)
#define bright_constant 500  // the gain constant from external light (0 - 1023)
// than the LESS constant, the "sharper" the brightness will be added
#define coef 0.9             // the filter coefficient (0.0 - 1.0), the more - the more slowly the brightness changes

int new_bright, new_bright_f;
unsigned long bright_timer, off_timer;

// DPsoftware Checksum
uint8_t prefix[] = {'D', 'P', 's'}, hi, lo, chk, loSecondPart, usbBrightness, gpio, i;
bool led_state = true;
uint lastLedUpdate = 10000;
uint lastStream = 0;
float framerate = 0;
float framerateCounter = 0;
int gpioInUse;

/****************** FastLED Defintions ******************/
#define NUM_LEDS    550 // Max Led support
CRGB leds[NUM_LEDS];
int dynamicLedNum = NUM_LEDS;
const String LED_NUM_FILENAME = "led_number.json";
const String GPIO_FILENAME = "gpio.json";
const String LED_NUM_PARAM = "lednum";
const String GPIO_PARAM = "gpio";

const int FIRST_CHUNK = 190;
const int SECOND_CHUNK = 380;
#define DATA_PIN    5 // Wemos D1 Mini Lite PIN D5
//#define CLOCK_PIN 5
#define CHIPSET     WS2812B
#define COLOR_ORDER GRB

byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

/****************** GLOBALS for fade/flash ******************/
bool stateOn = false;
bool startFade = false;
bool onbeforeflash = false;
unsigned long lastLoop = 0;
unsigned transitionTime = 0;
int effectSpeed = 0;
bool inFade = false;
int loopCount = 0;
int stepR, stepG, stepB;
int redVal, grnVal, bluVal;

bool flash = false;
bool startFlash = false;
unsigned int flashLength = 0;
unsigned long flashStartTime = 0;
byte flashRed = red;
byte flashGreen = green;
byte flashBlue = blue;
byte flashBrightness = brightness;

//RAINBOW
uint16_t thishue = 0; // Starting hue value.
uint16_t deltahue = 10;

//CANDYCANE
CRGBPalette16 currentPalettestriped; //for Candy Cane
CRGBPalette16 gPal; //for fire

//NOISE
#ifdef TARGET_GLOWWORMLUCIFERINFULL
static uint16_t dist;         // A random number for our noise generator.
#endif
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint16_t maxChanges = 48;      // Value for blending between palettes.
CRGBPalette16 targetPalette(OceanColors_p);
CRGBPalette16 currentPalette(CRGB::Black);

//TWINKLE
#define DENSITY     80
int twinklecounter = 0;

//RIPPLE
uint16_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint16_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
uint16_t bgcol = 0;                                            // Background colour rotates.
int thisdelay = 20;                                           // Standard delay value.

//DOTS
uint16_t   count =   0;                                        // Count up to 255 and then reverts to 0
uint16_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
uint16_t bpm = 30;

//LIGHTNING
uint16_t frequency = 50;                                       // controls the interval between strikes
uint16_t flashes = 8;                                          //the upper limit of flashes per strike
unsigned int dimmer = 1;
uint16_t ledstart;                                             // Starting location of a flash
uint16_t ledlen;
int lightningcounter = 0;

//FUNKBOX
int idex = 0;                //-LED INDEX (0 to NUM_LEDS-1
int TOP_INDEX = int(NUM_LEDS / 2);
int thissat = 255;           //-FX LOOPS DELAY VAR
uint16_t thishuepolice = 0;
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % NUM_LEDS;
  }
  return iN;
}

//FIRE
#define COOLING  55
#define SPARKING 120
bool gReverseDirection = false;

//BPM
uint16_t gHue = 0;

bool breakLoop = false;
int part = 1;

// Upgrade firmware
boolean firmwareUpgrade = false;
size_t updateSize = 0;

/****************** FUNCTION DECLARATION (NEEDED BY PLATFORMIO WHILE COMPILING CPP FILES) ******************/
// Bootstrap functions
void callback(char* topic, byte* payload, unsigned int length);
void manageDisconnections();
void manageQueueSubscription();
void manageHardwareButton();
// Project specific functions
void sendStatus();
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA);
bool processTimeJson(StaticJsonDocument<BUFFER_SIZE> json);
bool processUpdate(StaticJsonDocument<BUFFER_SIZE> json);
bool processJson(StaticJsonDocument<BUFFER_SIZE> json);
bool processGlowWormLuciferinRebootCmnd(StaticJsonDocument<BUFFER_SIZE> json);
void setColor(int inR, int inG, int inB);
void checkConnection();
void fadeall();
void Fire2012WithPalette();
void addGlitter(fract8 chanceOfGlitter);
void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue);
void showleds();
void temp2rgb(unsigned int kelvin);
int calculateStep(int prevValue, int endValue);
int calculateVal(int step, int val, int i);
void mainTask(void * parameter);
void mainLoop();
CRGB Scroll(int pos);
void sendSerialInfo();
void feedTheDog();