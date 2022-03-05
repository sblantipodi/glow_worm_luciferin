/*
  Helpers.cpp - Helper classes
  
  Copyright (C) 2020 - 2022  Davide Perini
  
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
*/

#include "Globals.h"
#include "BootstrapManager.h"
#include "EffectsManager.h"
#include "LedManager.h"
#include "NetworkManager.h"

BootstrapManager bootstrapManager;
EffectsManager effectsManager;
LedManager ledManager;
NetworkManager networkManager;
Helpers helper;
uint8_t gpioInUse = 2;
uint8_t colorMode = 0;
byte brightness = 255;
uint8_t whiteTempCorrection[] = {255, 255, 255};
Effect effect;
float framerate = 0;
float framerateCounter = 0;
uint lastStream = 0;
