#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FastLED.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Secrets.h>
#include <Version.h>


/************ WIFI and MQTT Information (CHANGE THESE FOR YOUR SETUP) ******************/
const int mqtt_port = 1883;

/**************************** FOR OTA **************************************************/
#define SENSORNAME "ambilight" //change this to whatever you want to call your device
int OTAport = 8266;
// the IP address for the shield:
IPAddress arduinoip(192, 168, 1, 52);
// the DNS address for the shield:
IPAddress mydns(192, 168, 1, 1);
// the GATEWAY address for the shield:
IPAddress mygateway(192, 168, 1, 1);


/************* MQTT TOPICS (change these topics as you wish)  **************************/
const char* light_state_topic = "lights/pcambilight";  //da configurare
const char* light_set_topic = "lights/pcambiligh/set";  //da configurare
const char* smartostat_climate_state_topic = "stat/smartostat/CLIMATE";
const char* cmnd_ambi_reboot = "cmnd/ambilight/reboot";
const char* stat_ambi_reboot = "stat/ambilight/reboot";

const char* on_cmd = "ON";
const char* off_cmd = "OFF";
const char* effect = "solid";
String effectString = "solid";
String oldeffectString = "solid";

/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

/********************************* AmbiLight *************************************/
#define max_bright 255       // maximum brightness (0 - 255)
#define min_bright 50        // the minimum brightness (0 - 255)
#define bright_constant 500  // the gain constant from external light (0 - 1023)
// than the LESS constant, the "sharper" the brightness will be added
#define coef 0.9             // the filter coefficient (0.0 - 1.0), the more - the more slowly the brightness changes

int new_bright, new_bright_f;
unsigned long bright_timer, off_timer;

#define serialRate 500000
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;
bool led_state = true;

/*********************************** FastLED Defintions ********************************/
#define NUM_LEDS    95 //95  //da configurare
#define DATA_PIN    5 // Su Wemos D1 Mini Lite sarebbe il PIN D5
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


WiFiClient espClient;
PubSubClient client(espClient);
CRGB leds[NUM_LEDS];

// LED_BUILTIN vars
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 200;           // interval at which to blink (milliseconds)
bool ledTriggered = false;
const int blinkTimes = 6; // 6 equals to 3 blink on and 3 off
int blinkCounter = 0;

String timedate = "OFF";

/********************************** START SETUP*****************************************/
void setup() {
  // LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  setupStripedPalette( CRGB::Red, CRGB::Red, CRGB::White, CRGB::White); //for CANDY CANE
  gPal = HeatColors_p; //for FIRE

  Serial.begin(serialRate);
  Serial.print(F("Ada\n"));     // Связаться с компом

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.print(F("Starting"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.print(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.print(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.print(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.print(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.print(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.print(F("End Failed"));
  });
  ArduinoOTA.begin();

  Serial.print(F("Ready"));
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

}

/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.persistent(false);   // Solve possible wifi init errors (re-add at 6.2.1.16 #4044, #4083)
  WiFi.disconnect(true);    // Delete SDK wifi config
  delay(200);
  WiFi.mode(WIFI_STA);      // Disable AP mode
  //WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  // IP of the arduino, dns, gateway
  WiFi.config(arduinoip, mydns, mygateway);

  WiFi.hostname(SENSORNAME);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.print(F(""));
  Serial.print(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}

/*
  SAMPLE PAYLOAD:
  {
    "brightness": 120,
    "color": {
      "r": 255,
      "g": 100,
      "b": 100
    },
    "flash": 2,
    "transition": 5,
    "state": "ON"
  }
*/



/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if(strcmp(topic, smartostat_climate_state_topic) == 0) {
    if (!processSmartostatClimateJson(message)) {
      return;
    }
  } else if(strcmp(topic, cmnd_ambi_reboot) == 0) {
    if (!processAmbilightRebootCmnd(message)) {
      return;
    }
  } else {
    if (!processJson(message)) {
      return;
    }
  }
  
  if (stateOn) {

    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {

    realRed = 0;
    realGreen = 0;
    realBlue = 0;
    if (effectString == "AmbiLight") {
      effect = "solid";
      effectString = "solid";
      oldeffectString = "solid";
      FastLED.clear();
      FastLED.show();
    }
  }

  Serial.println(effect);

  startFade = true;
  inFade = false; // Kill the current fade

  sendState();
}


bool processSmartostatClimateJson(char* message) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println(F("parseObject() failed 2"));
    return false;
  }

  if (doc.containsKey("smartostat")) {
    const char* timeConst = doc["Time"];
    timedate = timeConst;  
  }

  return true;
}

bool processAmbilightRebootCmnd(char* message) {
  String rebootState = message;
  if (rebootState == off_cmd) {     
    stateOn = false;   
    sendState(); 
    delay(1500);
    ESP.restart();
  }
  return true;
}

/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print(F("parseObject() failed"));
    return false;
  }

  if (doc.containsKey("state")) {
    if (strcmp(doc["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(doc["state"], off_cmd) == 0) {
      stateOn = false;
      onbeforeflash = false;
    }
  }

  // If "flash" is included, treat RGB and brightness differently
  if (doc.containsKey("flash")) {
    flashLength = (int)doc["flash"] * 1000;

    oldeffectString = effectString;

    if (doc.containsKey("brightness")) {
      flashBrightness = doc["brightness"];
    }
    else {
      flashBrightness = brightness;
    }

    if (doc.containsKey("color")) {
      flashRed = doc["color"]["r"];
      flashGreen = doc["color"]["g"];
      flashBlue = doc["color"]["b"];
    }
    else {
      flashRed = red;
      flashGreen = green;
      flashBlue = blue;
    }

    if (doc.containsKey("effect")) {
      effect = doc["effect"];
      effectString = effect;
      twinklecounter = 0; //manage twinklecounter
    }

    if (doc.containsKey("transition")) {
      transitionTime = doc["transition"];
    }
    else if ( effectString == "solid") {
      transitionTime = 0;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
    flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
    flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);

    flash = true;
    startFlash = true;
  }
  else { // Not flashing
    flash = false;

    if (stateOn) {   //if the light is turned on and the light isn't flashing
      onbeforeflash = true;
    }

    if (doc.containsKey("color")) {
      red = doc["color"]["r"];
      green = doc["color"]["g"];
      blue = doc["color"]["b"];
    }

    if (doc.containsKey("color_temp")) {
      //temp comes in as mireds, need to convert to kelvin then to RGB
      int color_temp = doc["color_temp"];
      //      unsigned int kelvin  = MILLION / color_temp;
      //
      //      temp2rgb(kelvin);

    }

    if (doc.containsKey("brightness")) {
      brightness = doc["brightness"];
    }

    if (doc.containsKey("effect")) {
      effect = doc["effect"];
      effectString = effect;
      twinklecounter = 0; //manage twinklecounter
    }

    if (doc.containsKey("transition")) {
      transitionTime = doc["transition"];
    }
    else if ( effectString == "solid") {
      transitionTime = 0;
    }

  }

  return true;
}



/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonDocument<BUFFER_SIZE> doc;

  JsonObject root = doc.to<JsonObject>();

  root["Whoami"] = SENSORNAME;
  root["IP"] = WiFi.localIP().toString();
  root["MAC"] = WiFi.macAddress();
  root["ver"] = VERSION_SHORT;
  
  root["Time"] = timedate;
  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;

  root["brightness"] = brightness;
  root["effect"] = effectString.c_str();

  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);

  // Built in led triggered
  ledTriggered = true;
}

// Blink LED_BUILTIN without bloking delay
void nonBlokingBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval && ledTriggered) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // blink led
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    blinkCounter++;
    if (blinkCounter >= blinkTimes) {
      blinkCounter = 0;
      ledTriggered = false;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }  
}


/********************************** START RECONNECT*****************************************/
void reconnect() {
  // variabile usata per tenere il conto di quanti tentativi di connessione al broker mqtt faccio
  int brokermqttcounter = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_username, mqtt_password)) {
      Serial.print(F("connected"));
      client.subscribe(light_set_topic);
      client.subscribe(smartostat_climate_state_topic);
      client.subscribe(cmnd_ambi_reboot);            
      // ogni tanto quando HA smette di rispondere si disconnette per 10 secondi e mi spegne i led tolto il set state a 0
      //setColor(0, 0, 0);
      //sendState();
      brokermqttcounter = 0;
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.print(F(" try again in 5 seconds"));
      Serial.print(F("Numbers of retry="));
      Serial.println(brokermqttcounter);
      // se dopo 10 tentativi di connessione al broker mqtt non ottengo risposta, spengo i led forzatamente.
      if (brokermqttcounter == 10) {
        setColor(0, 0, 0);
        sendState();
      }
      brokermqttcounter++;
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



/********************************** START Set Color*****************************************/
void setColor(int inR, int inG, int inB) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].red   = inR;
    leds[i].green = inG;
    leds[i].blue  = inB;
  }

  FastLED.show();

  Serial.print(F("Setting LEDs:"));
  Serial.print(F("r: "));
  Serial.print(inR);
  Serial.print(F(", g: "));
  Serial.print(inG);
  Serial.print(F(", b: "));
  Serial.println(inB);
}


void check_connection() {
  if (!client.connected()) {
    reconnect();
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
    setup_wifi();
    return;
  }
  client.loop();

}


/********************************** START MAIN LOOP*****************************************/
void loop() {

  if (!client.connected()) {
    reconnect();
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
    setup_wifi();
    return;
  }
  client.loop();
  
  ArduinoOTA.handle();

  nonBlokingBlink();

  //EFFECT BPM
  if (effectString == "AmbiLight") {
    if (!led_state) led_state = true;
    off_timer = millis();

    for (i = 0; i < sizeof prefix; ++i) {
waitLoop: while (!Serial.available()) check_connection();;
      if (prefix[i] == Serial.read()) continue;
      i = 0;
      goto waitLoop;
    }

    while (!Serial.available()) check_connection();;
    hi = Serial.read();
    while (!Serial.available()) check_connection();;
    lo = Serial.read();
    while (!Serial.available()) check_connection();;
    chk = Serial.read();
    if (chk != (hi ^ lo ^ 0x55))
    {
      i = 0;
      goto waitLoop;
    }

    memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      byte r, g, b;
      // читаем данные для каждого цвета
      while (!Serial.available()) check_connection();
      r = Serial.read();
      while (!Serial.available()) check_connection();
      g = Serial.read();
      while (!Serial.available()) check_connection();
      b = Serial.read();
      leds[i].r = r;
      leds[i].g = g;
      leds[i].b = b;
    }
    FastLED.show();  // записываем цвета в ленту
    // Flush serial buffer
    while(Serial.available() > 0) {
      Serial.read();
    }
  }

  //EFFECT BPM
  if (effectString == "bpm") {
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < NUM_LEDS; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }


  //EFFECT Candy Cane
  if (effectString == "candy cane") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */
    fill_palette( leds, NUM_LEDS,
                  startIndex, 16, /* higher = narrower stripes */
                  currentPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    showleds();
  }


  //EFFECT CONFETTI
  if (effectString == "confetti" ) {
    fadeToBlackBy( leds, NUM_LEDS, 25);
    int pos = random16(NUM_LEDS);
    leds[pos] += CRGB(realRed + random8(64), realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }


  //EFFECT CYCLON RAINBOW
  if (effectString == "cyclon rainbow") {                    //Single Dot Down
    static uint8_t hue = 0;
    // First slide the led in one direction
    for (int i = 0; i < NUM_LEDS; i++) {
      // Set the i'th led to red
      leds[i] = CHSV(hue++, 255, 255);
      // Show the leds
      showleds();
      // now that we've shown the leds, reset the i'th led to black
      // leds[i] = CRGB::Black;
      fadeall();
      // Wait a little bit before we loop around and do it again
      delay(10);
    }
    for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
      // Set the i'th led to red
      leds[i] = CHSV(hue++, 255, 255);
      // Show the leds
      showleds();
      // now that we've shown the leds, reset the i'th led to black
      // leds[i] = CRGB::Black;
      fadeall();
      // Wait a little bit before we loop around and do it again
      delay(10);
    }
  }


  //EFFECT DOTS
  if (effectString == "dots") {
    uint8_t inner = beatsin8(bpm, NUM_LEDS / 4, NUM_LEDS / 4 * 3);
    uint8_t outer = beatsin8(bpm, 0, NUM_LEDS - 1);
    uint8_t middle = beatsin8(bpm, NUM_LEDS / 3, NUM_LEDS / 3 * 2);
    leds[middle] = CRGB::Purple;
    leds[inner] = CRGB::Blue;
    leds[outer] = CRGB::Aqua;
    nscale8(leds, NUM_LEDS, fadeval);

    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }


  //EFFECT FIRE
  if (effectString == "fire") {
    Fire2012WithPalette();
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 150;
    }
    showleds();
  }

  random16_add_entropy( random8());


  //EFFECT Glitter
  if (effectString == "glitter") {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    addGlitterColor(80, realRed, realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }


  //EFFECT JUGGLE
  if (effectString == "juggle" ) {                           // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, NUM_LEDS, 20);
    for (int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, NUM_LEDS - 1  )] |= CRGB(realRed, realGreen, realBlue);
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    showleds();
  }


  //EFFECT LIGHTNING
  if (effectString == "lightning") {
    twinklecounter = twinklecounter + 1;                     //Resets strip if previous animation was running
    if (twinklecounter < 2) {
      FastLED.clear();
      FastLED.show();
    }
    ledstart = random8(NUM_LEDS);           // Determine starting location of flash
    ledlen = random8(NUM_LEDS - ledstart);  // Determine length of flash (not to go beyond NUM_LEDS-1)
    for (int flashCounter = 0; flashCounter < random8(3, flashes); flashCounter++) {
      if (flashCounter == 0) dimmer = 5;    // the brightness of the leader is scaled down by a factor of 5
      else dimmer = random8(1, 3);          // return strokes are brighter than the leader
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 255 / dimmer));
      showleds();    // Show a section of LED's
      delay(random8(4, 10));                // each flash only lasts 4-10 milliseconds
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 0)); // Clear the section of LED's
      showleds();
      if (flashCounter == 0) delay (130);   // longer delay until next flash after the leader
      delay(50 + random8(100));             // shorter delay between strokes
    }
    delay(random8(frequency) * 100);        // delay between strikes
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    showleds();
  }


  //EFFECT POLICE ALL
  if (effectString == "police all") {                 //POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= NUM_LEDS) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    leds[idexR] = CHSV(thishuepolice, thissat, 255);
    leds[idexB] = CHSV(thathue, thissat, 255);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }

  //EFFECT POLICE ONE
  if (effectString == "police one") {
    idex++;
    if (idex >= NUM_LEDS) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    for (int i = 0; i < NUM_LEDS; i++ ) {
      if (i == idexR) {
        leds[i] = CHSV(thishuepolice, thissat, 255);
      }
      else if (i == idexB) {
        leds[i] = CHSV(thathue, thissat, 255);
      }
      else {
        leds[i] = CHSV(0, 0, 0);
      }
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    showleds();
  }


  //EFFECT RAINBOW
  if (effectString == "rainbow") {
    // FastLED's built-in rainbow generator
    static uint8_t starthue = 0;    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    showleds();
  }

  //SOLID RAINBOW
  if (effectString == "solid rainbow") {
    // FastLED's built-in rainbow generator
    static uint8_t starthue = 0;    thishue++;
    fill_solid(leds, NUM_LEDS, CHSV(thishue,255,255));
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 40;
    }
    if (transitionTime < 130) {
      delay(130-transitionTime);
    }
    showleds();
  }

  //EFFECT RAINBOW WITH GLITTER
  if (effectString == "rainbow with glitter") {               // FastLED's built-in rainbow generator with Glitter
    static uint8_t starthue = 0;
    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
    addGlitter(80);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    showleds();
  }


  //EFFECT SIENLON
  if (effectString == "sinelon") {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS - 1);
    leds[pos] += CRGB(realRed, realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 150;
    }
    showleds();
  }


  //EFFECT TWINKLE
  if (effectString == "twinkle") {
    twinklecounter = twinklecounter + 1;
    if (twinklecounter < 2) {                               //Resets strip if previous animation was running
      FastLED.clear();
      FastLED.show();
    }
    const CRGB lightcolor(8, 7, 1);
    for ( int i = 0; i < NUM_LEDS; i++) {
      if ( !leds[i]) continue; // skip black pixels
      if ( leds[i].r & 1) { // is red odd?
        leds[i] -= lightcolor; // darken if red is odd
      } else {
        leds[i] += lightcolor; // brighten if red is even
      }
    }
    if ( random8() < DENSITY) {
      int j = random16(NUM_LEDS);
      if ( !leds[j] ) leds[j] = lightcolor;
    }

    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    showleds();
  }


  EVERY_N_MILLISECONDS(10) {

    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
    {
      gHue++;
    }

    //EFFECT NOISE
    if (effectString == "noise") {
      for (int i = 0; i < NUM_LEDS; i++) {                                     // Just onE loop to fill up the LED array as all of the pixels change.
        uint8_t index = inoise8(i * scale, dist + i * scale) % 255;            // Get a value from the noise function. I'm using both x and y axis.
        leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
      }
      dist += beatsin8(10, 1, 4);                                              // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
      // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
      if (transitionTime == 0 or transitionTime == NULL) {
        transitionTime = 0;
      }
      showleds();
    }

    //EFFECT RIPPLE
    if (effectString == "ripple") {
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(bgcol++, 255, 15);  // Rotate background colour.
      switch (step) {
        case -1:                                                          // Initialize ripple variables.
          center = random(NUM_LEDS);
          colour = random8();
          step = 0;
          break;
        case 0:
          leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
          step ++;
          break;
        case maxsteps:                                                    // At the end of the ripples.
          step = -1;
          break;
        default:                                                             // Middle of the ripples.
          leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller
          leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade / step * 2);
          step ++;                                                         // Next step.
          break;
      }
      if (transitionTime == 0 or transitionTime == NULL) {
        transitionTime = 30;
      }
      showleds();
    }

  }


  EVERY_N_SECONDS(5) {
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
  }


  //FLASH AND FADE SUPPORT
  if (flash) {
    if (startFlash) {
      startFlash = false;
      flashStartTime = millis();
    }

    if ((millis() - flashStartTime) <= flashLength) {
      if ((millis() - flashStartTime) % 1000 <= 500) {
        setColor(flashRed, flashGreen, flashBlue);
      }
      else {
        setColor(0, 0, 0);
        // If you'd prefer the flashing to happen "on top of"
        // the current color, uncomment the next line.
        // setColor(realRed, realGreen, realBlue);
      }
    }
    else {
      flash = false;
      effectString = oldeffectString;
      if (onbeforeflash) { //keeps light off after flash if light was originally off
        setColor(realRed, realGreen, realBlue);
      }
      else {
        stateOn = false;
        setColor(0, 0, 0);
        sendState();
      }
    }
  }

  if (startFade && effectString == "solid") {
    // If we don't want to fade, skip it.
    if (transitionTime == 0) {
      setColor(realRed, realGreen, realBlue);

      redVal = realRed;
      grnVal = realGreen;
      bluVal = realBlue;

      startFade = false;
    }
    else {
      loopCount = 0;
      stepR = calculateStep(redVal, realRed);
      stepG = calculateStep(grnVal, realGreen);
      stepB = calculateStep(bluVal, realBlue);

      inFade = true;
    }
  }

  if (inFade) {
    startFade = false;
    unsigned long now = millis();
    if (now - lastLoop > transitionTime) {
      if (loopCount <= 1020) {
        lastLoop = now;

        redVal = calculateVal(stepR, redVal, loopCount);
        grnVal = calculateVal(stepG, grnVal, loopCount);
        bluVal = calculateVal(stepB, bluVal, loopCount);

        if (effectString == "solid") {
          setColor(redVal, grnVal, bluVal); // Write current values to LED pins
        }
        loopCount++;
      }
      else {
        inFade = false;
      }
    }
  }
}


/**************************** START TRANSITION FADER *****************************************/
// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
  The program works like this:
  Imagine a crossfade that moves the red LED from 0-10,
    the green from 0-5, and the blue from 10 to 7, in
    ten steps.
    We'd want to count the 10 steps and increase or
    decrease color values in evenly stepped increments.
    Imagine a + indicates raising a value by 1, and a -
    equals lowering it. Our 10 step fade would look like:
    1 2 3 4 5 6 7 8 9 10
  R + + + + + + + + + +
  G   +   +   +   +   +
  B     -     -     -
  The red rises from 0 to 10 in ten steps, the green from
  0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
  In the real program, the color percentages are converted to
  0-255 values, and there are 1020 steps (255*4).
  To figure out how big a step there should be between one up- or
  down-tick of one of the LED values, we call calculateStep(),
  which calculates the absolute gap between the start and end values,
  and then divides that gap by 1020 to determine the size of the step
  between adjustments in the value.
*/
int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero,
    step = 1020 / step;          //   divide by 1020
  }

  return step;
}
/* The next function is calculateVal. When the loop value, i,
   reaches the step size appropriate for one of the
   colors, it increases or decreases the value of that color by 1.
   (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {
  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    }
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }

  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }

  return val;
}



/**************************** START STRIPLED PALETTE *****************************************/
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA) {
  currentPalettestriped = CRGBPalette16(
                            A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                            //    A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                          );
}



/********************************** START FADE************************************************/
void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);  //for CYCLon
  }
}



/********************************** START FIRE **********************************************/
void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}



/********************************** START ADD GLITTER *********************************************/
void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}



/********************************** START ADD GLITTER COLOR ****************************************/
void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB(red, green, blue);
  }
}



/********************************** START SHOW LEDS ***********************************************/
void showleds() {

  delay(1);

  if (stateOn) {
    FastLED.setBrightness(brightness);  //EXECUTE EFFECT COLOR
    FastLED.show();
    if (transitionTime > 0 && transitionTime < 130) {  //Sets animation speed based on receieved value
      FastLED.delay(1000 / transitionTime);
      //delay(10*transitionTime);
    }
  }
  else if (startFade) {
    setColor(0, 0, 0);
    startFade = false;
  }
}
void temp2rgb(unsigned int kelvin) {
  int tmp_internal = kelvin / 100.0;

  // red
  if (tmp_internal <= 66) {
    red = 255;
  } else {
    float tmp_red = 329.698727446 * pow(tmp_internal - 60, -0.1332047592);
    if (tmp_red < 0) {
      red = 0;
    } else if (tmp_red > 255) {
      red = 255;
    } else {
      red = tmp_red;
    }
  }

  // green
  if (tmp_internal <= 66) {
    float tmp_green = 99.4708025861 * log(tmp_internal) - 161.1195681661;
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  } else {
    float tmp_green = 288.1221695283 * pow(tmp_internal - 60, -0.0755148492);
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  }

  // blue
  if (tmp_internal >= 66) {
    blue = 255;
  } else if (tmp_internal <= 19) {
    blue = 0;
  } else {
    float tmp_blue = 138.5177312231 * log(tmp_internal - 10) - 305.0447927307;
    if (tmp_blue < 0) {
      blue = 0;
    } else if (tmp_blue > 255) {
      blue = 255;
    } else {
      blue = tmp_blue;
    }
  }
}
