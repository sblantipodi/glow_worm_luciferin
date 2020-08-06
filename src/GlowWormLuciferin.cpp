/*
  GlowWormLuciferin.cpp - Glow Worm Luciferin for Firefly Luciferin
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

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "GlowWormLuciferin.h"

/********************************** START SETUP *****************************************/
void setup() {

  Serial.begin(SERIAL_RATE);

  // Pin configuration
  pinMode(LED_BUILTIN, OUTPUT);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  setupStripedPalette(CRGB::Red, CRGB::Red, CRGB::White, CRGB::White); //for CANDY CANE
  gPal = HeatColors_p; //for FIRE

  #ifdef TARGET_GLOWWORMLUCIFERINFULL
    // Bootsrap setup() with Wifi and MQTT functions
    bootstrapManager.bootstrapSetup(manageDisconnections, manageHardwareButton, callback);
  #endif

}

#ifdef TARGET_GLOWWORMLUCIFERINFULL

  /********************************** MANAGE WIFI AND MQTT DISCONNECTION *****************************************/
  void manageDisconnections() {

    setColor(0, 0, 0);

  }

  /********************************** MQTT SUBSCRIPTIONS *****************************************/
  void manageQueueSubscription() {

    bootstrapManager.subscribe(LIGHT_SET_TOPIC);
    bootstrapManager.subscribe(SMARTOSTAT_CLIMATE_STATE_TOPIC);
    bootstrapManager.subscribe(CMND_AMBI_REBOOT);

  }

  /********************************** MANAGE HARDWARE BUTTON *****************************************/
  void manageHardwareButton() {
    // no hardware button at the moment
  }

  /********************************** START CALLBACK *****************************************/
  void callback(char* topic, byte* payload, unsigned int length) {

    StaticJsonDocument<BUFFER_SIZE> json = bootstrapManager.parseQueueMsg(topic, payload, length);

    if(strcmp(topic, SMARTOSTAT_CLIMATE_STATE_TOPIC) == 0) {
      processSmartostatClimateJson(json);
    } else if(strcmp(topic, CMND_AMBI_REBOOT) == 0) {
      processGlowWormLuciferinRebootCmnd(json);
    } else {
      processJson(json);
    }

    if (stateOn) {
      realRed = map(red, 0, 255, 0, brightness);
      realGreen = map(green, 0, 255, 0, brightness);
      realBlue = map(blue, 0, 255, 0, brightness);
    } else {
      realRed = 0;
      realGreen = 0;
      realBlue = 0;
      if (effectString == "GlowWorm") {
        effect = "solid";
        effectString = "solid";
        oldeffectString = "solid";
        FastLED.clear();
        FastLED.show();
      }
    }

    Serial.print(F("Effect= ")); Serial.println(effect);
    startFade = true;
    inFade = false; // Kill the current fade
    sendStatus();

  }

  /********************************** START PROCESS JSON *****************************************/
  bool processJson(StaticJsonDocument<BUFFER_SIZE> json) {

    lastLedUpdate = millis();

    if (json.containsKey("state")) {
      String state = json["state"];
      if (state == ON_CMD) {
        stateOn = true;
      } else if (state == OFF_CMD) {
        stateOn = false;
        onbeforeflash = false;
      }
    }

    // If "flash" is included, treat RGB and brightness differently
    if (json.containsKey("flash")) {
      flashLength = (int)json["flash"] * 1000;

      oldeffectString = effectString;

      if (json.containsKey("brightness")) {
        flashBrightness = json["brightness"];
      } else {
        flashBrightness = brightness;
      }

      if (json.containsKey("color")) {
        flashRed = json["color"]["r"];
        flashGreen = json["color"]["g"];
        flashBlue = json["color"]["b"];
      } else {
        flashRed = red;
        flashGreen = green;
        flashBlue = blue;
      }

      if (json.containsKey("effect")) {
        effect = json["effect"];
        effectString = effect;
        twinklecounter = 0; //manage twinklecounter
      }

      if (json.containsKey("transition")) {
        transitionTime = json["transition"];
      } else if ( effectString == "solid") {
        transitionTime = 0;
      }

      flashRed = map(flashRed, 0, 255, 0, flashBrightness);
      flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
      flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);
      flash = true;
      startFlash = true;

    } else { // Not flashing

      flash = false;
      if (stateOn) {   //if the light is turned on and the light isn't flashing
        onbeforeflash = true;
      }

      if (json.containsKey("color")) {
        red = json["color"]["r"];
        green = json["color"]["g"];
        blue = json["color"]["b"];
      }

      if (json.containsKey("brightness")) {
        brightness = json["brightness"];
      }

      if (json.containsKey("effect")) {
        effect = json["effect"];
        effectString = effect;
        twinklecounter = 0; //manage twinklecounter
      }

      if (json.containsKey("transition")) {
        transitionTime = json["transition"];
      } else if ( effectString == "solid") {
        transitionTime = 0;
      }

    }

    return true;

  }

  /********************************** START SEND STATE*****************************************/
  void sendStatus() {

    JsonObject root = bootstrapManager.getJsonObject();
    root["state"] = (stateOn) ? ON_CMD : OFF_CMD;

    JsonObject color = root.createNestedObject("color");
    color["r"] = red;
    color["g"] = green;
    color["b"] = blue;

    root["brightness"] = brightness;
    root["effect"] = effectString.c_str();

    bootstrapManager.sendState(LIGHT_STATE_TOPIC, root, VERSION);

    // Built in led triggered
    ledTriggered = true;

  }

  /* Get Time Info from MQTT queue, you can remove this part if you don't need it. I use it for monitoring
     NOTE: This is specific of my home "ecosystem", I prefer to take time from my internal network and not from the internet, you can delete this if you don't need it.
     Or you can take your time in this function.
  */
  bool processSmartostatClimateJson(StaticJsonDocument<BUFFER_SIZE> json) {

    if (json.containsKey("smartostat")) {
      const char* timeConst = json["Time"];
      timedate = timeConst;
    }
    return true;

  }

  // MQTT reboot command
  bool processGlowWormLuciferinRebootCmnd(StaticJsonDocument<BUFFER_SIZE> json) {

    if (json[VALUE] == OFF_CMD) {
      stateOn = false;
      sendStatus();
      delay(1500);
      ESP.restart();
    }
    return true;

  }

#endif

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


void checkConnection() {
#ifdef TARGET_GLOWWORMLUCIFERINFULL
  // Bootsrap loop() with Wifi, MQTT and OTA functions
  bootstrapManager.bootstrapLoop(manageDisconnections, manageQueueSubscription, manageHardwareButton);
  EVERY_N_SECONDS(15) {
    // No updates since 15 seconds, turn off LEDs
    if(!breakLoop && (effectString == "GlowWorm") && (millis() > lastLedUpdate + 10000)){
      breakLoop = true;
      effectString = "solid";
      stateOn = ON_CMD;
      bootstrapManager.publish(LIGHT_SET_TOPIC, helper.string2char("{\"state\": \"ON\", \"effect\": \"solid\"}"), false);
      delay(DELAY_500);
    }
  }
#elif  TARGET_GLOWWORMLUCIFERINLIGHT
    EVERY_N_SECONDS(15) {
      // No updates since 15 seconds, turn off LEDs
      if(millis() > lastLedUpdate + 10000){
        setColor(0, 0, 0);
      }
    }
#endif
}

int serialRead() {
  return !breakLoop ? Serial.read() : -1;
}

/********************************** START MAIN LOOP *****************************************/
void loop() {

  #ifdef TARGET_GLOWWORMLUCIFERINFULL
    checkConnection();
  #endif
  bootstrapManager.nonBlokingBlink();


  // GLOW_WORM_LUCIFERIN, serial connection with Firefly Luciferin
  #ifdef TARGET_GLOWWORMLUCIFERINFULL
  if (effectString == "GlowWorm") {
  #endif
    if (!led_state) led_state = true;
    off_timer = millis();

    for (i = 0; i < sizeof prefix; ++i) {
      waitLoop: while (!breakLoop && !Serial.available()) checkConnection();
      if (breakLoop || prefix[i] == serialRead()) continue;
      i = 0;
      goto waitLoop;
    }

    while (!breakLoop && !Serial.available()) checkConnection();
    hi = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    lo = serialRead();
    while (!breakLoop && !Serial.available()) checkConnection();
    chk = serialRead();

    if (!breakLoop && (chk != (hi ^ lo ^ 0x55))) {
      i = 0;
      goto waitLoop;
    }

    memset(leds, 0, (lo+1) * sizeof(struct CRGB));
    for (uint8_t i = 0; i < (lo+1); i++) {
      byte r, g, b;
      while (!breakLoop && !Serial.available()) checkConnection();
      r = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      g = serialRead();
      while (!breakLoop && !Serial.available()) checkConnection();
      b = serialRead();
      leds[i].r = r;
      leds[i].g = g;
      leds[i].b = b;
    }
    lastLedUpdate = millis();
    FastLED.show();
    // Flush serial buffer
    while(!breakLoop && Serial.available() > 0) {
      serialRead();
    }
    breakLoop = false;
  #ifdef TARGET_GLOWWORMLUCIFERINFULL
  }
  #endif
  #ifdef TARGET_GLOWWORMLUCIFERINFULL

    //EFFECT BPM
    if (effectString == "bpm") {
      uint8_t BeatsPerMinute = 62;
      CRGBPalette16 palette = PartyColors_p;
      uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
      for ( int i = 0; i < NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
      }
      if (transitionTime == 0) {
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
      if (transitionTime == 0) {
        transitionTime = 0;
      }
      showleds();
    }

    //EFFECT CONFETTI
    if (effectString == "confetti" ) {
      fadeToBlackBy( leds, NUM_LEDS, 25);
      int pos = random16(NUM_LEDS);
      leds[pos] += CRGB(realRed + random8(64), realGreen, realBlue);
      if (transitionTime == 0) {
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

      if (transitionTime == 0) {
        transitionTime = 30;
      }
      showleds();
    }

    //EFFECT FIRE
    if (effectString == "fire") {
      Fire2012WithPalette();
      if (transitionTime == 0) {
        transitionTime = 150;
      }
      showleds();
    }
    random16_add_entropy( random8());

    //EFFECT Glitter
    if (effectString == "glitter") {
      fadeToBlackBy( leds, NUM_LEDS, 20);
      addGlitterColor(80, realRed, realGreen, realBlue);
      if (transitionTime == 0) {
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
      if (transitionTime == 0) {
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
      if (transitionTime == 0) {
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
      if (transitionTime == 0) {
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
      if (transitionTime == 0) {
        transitionTime = 30;
      }
      showleds();
    }

    //EFFECT RAINBOW
    if (effectString == "rainbow") {
      // FastLED's built-in rainbow generator
      thishue++;
      fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
      if (transitionTime == 0) {
        transitionTime = 130;
      }
      showleds();
    }

    //SOLID RAINBOW
    if (effectString == "solid rainbow") {
      // FastLED's built-in rainbow generator
      thishue++;
      fill_solid(leds, NUM_LEDS, CHSV(thishue,255,255));
      if (transitionTime == 0) {
        transitionTime = 40;
      }
      if (transitionTime < 130) {
        delay(130-transitionTime);
      }
      showleds();
    }

    //EFFECT RAINBOW WITH GLITTER
    if (effectString == "rainbow with glitter") {               // FastLED's built-in rainbow generator with Glitter
      thishue++;
      fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
      addGlitter(80);
      if (transitionTime == 0) {
        transitionTime = 130;
      }
      showleds();
    }

    //EFFECT SIENLON
    if (effectString == "sinelon") {
      fadeToBlackBy( leds, NUM_LEDS, 20);
      int pos = beatsin16(13, 0, NUM_LEDS - 1);
      leds[pos] += CRGB(realRed, realGreen, realBlue);
      if (transitionTime == 0) {
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

      if (transitionTime == 0) {
        transitionTime = 0;
      }
      showleds();
    }

    //EVERY 10 MINUTES
    EVERY_N_SECONDS(600) {
      // Ping gateway to add presence on the routing table,
      // command is synchrounous and adds a bit of lag to the loop
      Ping.ping(IP_DNS, 1);
    }

    //EVERY 10 MILLISECONDS
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
        if (transitionTime == 0) {
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
        if (transitionTime == 0) {
          transitionTime = 30;
        }
        showleds();
      }

    }

    // EVERY 5 SECONDS
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
          sendStatus();
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
  #endif

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
  );

}

/********************************** START FADE************************************************/
void fadeall() {

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);  //for CYCLon
  }

}

/********************************** START FIRE **********************************************/
void Fire2012WithPalette() {

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
void addGlitter(fract8 chanceOfGlitter) {

  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }

}

/********************************** START ADD GLITTER COLOR ****************************************/
void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue) {

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
