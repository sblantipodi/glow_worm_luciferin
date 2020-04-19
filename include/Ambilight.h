#ifndef PC_AMBILIGHT_H
#define PC_AMBILIGHT_H

#include <FastLED.h>
#include <Version.h>
#include "../arduino_bootstrapper/core/BootstrapManager.h"

/****************** BOOTSTRAP MANAGER ******************/
BootstrapManager bootstrapManager;

/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char* light_state_topic = "lights/pcambilight";  
const char* light_set_topic = "lights/pcambiligh/set";  
const char* smartostat_climate_state_topic = "stat/smartostat/CLIMATE";
const char* cmnd_ambi_reboot = "cmnd/ambilight/reboot";
const char* stat_ambi_reboot = "stat/ambilight/reboot";

const char* effect = "solid";
String effectString = "solid";
String oldeffectString = "solid";

/********************************* AmbiLight *************************************/
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
int transitionTime = 0;
int effectSpeed = 0;
bool inFade = false;
int loopCount = 0;
int stepR, stepG, stepB;
int redVal, grnVal, bluVal;

bool flash = false;
bool startFlash = false;
int flashLength = 0;
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

// LED_BUILTIN vars
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 200;           // interval at which to blink (milliseconds)
bool ledTriggered = false;
const int blinkTimes = 6; // 6 equals to 3 blink on and 3 off
int blinkCounter = 0;

String timedate = "OFF";

/********************************** FUNCTION DECLARATION (NEEDED BY PLATFORMIO WHILE COMPILING CPP FILES) *****************************************/
void manageDisconnections();
void manageQueueSubscription();
void manageHardwareButton();
void sendStatus();
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA);
void callback(char* topic, byte* payload, unsigned int length);
bool processSmartostatClimateJson(char* message);
bool processJson(char* message);
void nonBlokingBlink();
bool processAmbilightRebootCmnd(char* message);
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

#endif