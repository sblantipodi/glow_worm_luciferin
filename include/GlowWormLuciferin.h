/*
  GlowWormLuciferin.h - Glow Worm Luciferin for Firefly Luciferin
  All in one Bias Lighting system for PC
  
  Copyright (C) 2020  Davide Perini
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of 
  this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.
  
  You should have received a copy of the MIT License along with this program.  
  If not, see <https://opensource.org/licenses/MIT/>.

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
#include <ESP8266Ping.h>


/****************** BOOTSTRAP MANAGER ******************/
BootstrapManager bootstrapManager;
Helpers helper;

/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char* LIGHT_STATE_TOPIC = "lights/pcambilight";  
const char* LIGHT_SET_TOPIC = "lights/glowwormluciferin/set";
const char* SMARTOSTAT_CLIMATE_STATE_TOPIC = "stat/smartostat/CLIMATE";
const char* CMND_AMBI_REBOOT = "cmnd/ambilight/reboot";

const char* effect = "solid";
String effectString = "solid";
String oldeffectString = "solid";

/********************************* Glow Worm Luciferin *************************************/
#define max_bright 255       // maximum brightness (0 - 255)
#define min_bright 50        // the minimum brightness (0 - 255)
#define bright_constant 500  // the gain constant from external light (0 - 1023)
// than the LESS constant, the "sharper" the brightness will be added
#define coef 0.9             // the filter coefficient (0.0 - 1.0), the more - the more slowly the brightness changes

int new_bright, new_bright_f;
unsigned long bright_timer, off_timer;

uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;
bool led_state = true;

/*********************************** FastLED Defintions ********************************/
#define NUM_LEDS    95 //95  
#define DATA_PIN    5 // Wemos D1 Mini Lite PIN D5
//#define CLOCK_PIN 5
#define CHIPSET     WS2812
#define COLOR_ORDER GRB

byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 255;

/******************************** GLOBALS for fade/flash *******************************/
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

/********************************** GLOBALS for EFFECTS ******************************/
//RAINBOW
uint8_t thishue = 0;                                          // Starting hue value.
uint8_t deltahue = 10;

//CANDYCANE
CRGBPalette16 currentPalettestriped; //for Candy Cane
CRGBPalette16 gPal; //for fire

//NOISE
static uint16_t dist;         // A random number for our noise generator.
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.
CRGBPalette16 targetPalette(OceanColors_p);
CRGBPalette16 currentPalette(CRGB::Black);

//TWINKLE
#define DENSITY     80
int twinklecounter = 0;

//RIPPLE
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
uint8_t bgcol = 0;                                            // Background colour rotates.
int thisdelay = 20;                                           // Standard delay value.

//DOTS
uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
uint8_t bpm = 30;

//LIGHTNING
uint8_t frequency = 50;                                       // controls the interval between strikes
uint8_t flashes = 8;                                          //the upper limit of flashes per strike
unsigned int dimmer = 1;
uint8_t ledstart;                                             // Starting location of a flash
uint8_t ledlen;
int lightningcounter = 0;

//FUNKBOX
int idex = 0;                //-LED INDEX (0 to NUM_LEDS-1
int TOP_INDEX = int(NUM_LEDS / 2);
int thissat = 255;           //-FX LOOPS DELAY VAR
uint8_t thishuepolice = 0;
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
uint8_t gHue = 0;
CRGB leds[NUM_LEDS];

/********************************** FUNCTION DECLARATION (NEEDED BY PLATFORMIO WHILE COMPILING CPP FILES) *****************************************/
// Bootstrap functions
void callback(char* topic, byte* payload, unsigned int length);
void manageDisconnections();
void manageQueueSubscription();
void manageHardwareButton();
// Project specific functions
void sendStatus();
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA);
bool processSmartostatClimateJson(StaticJsonDocument<BUFFER_SIZE> json);
bool processJson(StaticJsonDocument<BUFFER_SIZE> json);
bool processGlowWormLuciferinRebootCmnd(StaticJsonDocument<BUFFER_SIZE> json);
void setColor(int inR, int inG, int inB);
void checkConnection();
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA);
void fadeall();
void Fire2012WithPalette();
void addGlitter(fract8 chanceOfGlitter);
void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue);
void showleds();
void temp2rgb(unsigned int kelvin);
int calculateStep(int prevValue, int endValue);
int calculateVal(int step, int val, int i);