/*
  WebSettings.h - Glow Worm Luciferin for Firefly Luciferin
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

#ifndef GLOW_WORM_LUCIFERIN_WEBSETTINGS_H
#define GLOW_WORM_LUCIFERIN_WEBSETTINGS_H

const char settingsPage[] PROGMEM = R"=====(<!DOCTYPE html><html><head> <title>LUCIFERIN Web Interface</title> <meta charset='utf-8'> <meta name='viewport' content='width=device-width, initial-scale=1'> <link rel="icon" type="image/png" href="https://sblantipodi.github.io/glow_worm_luciferin/static/logos/luciferin_logo.png" > <script src='https://sblantipodi.github.io/glow_worm_luciferin/third_party_scripts/jquery-3.6.0.slim.min.js' integrity='sha256-u7e5khyithlIdTpu22PHhENmPcRdFiHRjhAuHcs05RI=' crossorigin='anonymous'></script> <link href='https://sblantipodi.github.io/glow_worm_luciferin/third_party_scripts/bootstrap.min.css' rel='stylesheet'> <script src='https://sblantipodi.github.io/glow_worm_luciferin/third_party_scripts/bootstrap.bundle.min.js'></script> <script src='https://sblantipodi.github.io/glow_worm_luciferin/third_party_scripts/iro5_2_2.js'></script> <link href='https://sblantipodi.github.io/glow_worm_luciferin/webInterface.css' rel='stylesheet'></head><body><div class='container-fluid p-3 bg-primary text-white text-center'> <h1>LUCIFERIN</h1> <p>Bias Lighting and Ambient Light firmware designed for Firefly Luciferin </p></div><div class='container'> <div class='row'> <div class='col' style='margin-top: 2vw;'> <form onsubmit='submitForm(event)'> <div class='form-group row'> <div class='col-sm-12'> <div class='wrap'> <div id='picker'></div></div></div><div class='col-sm-12'> <button id='toggleLED' type='button' class='btn btn-primary' data-bs-toggle='button' autocomplete='off'>Turn ON</button> </div><div class='col-sm-12'> <div class='form-floating'> <select class='form-select' id='effectSelect'> </select> <label for='effectSelect'>EFFECT</label> </div></div><div class='col-sm-12'> <div class='form-floating'> <select class='form-select' id='whiteTempSelect'> </select> <label for='whiteTempSelect'>WHITE TEMPERATURE</label> </div></div></div></form> </div></div></div><footer class='bg-light text-center text-lg-start'> <div class='text-center'> <small> WiFi: <span id='wifi'>_</span>/ Framerate: <span id='fps'>_</span><br><a id='gitlink' class='text-dark' href='https://github.com/sblantipodi/'>_</a></small> </div></footer><script src='https://sblantipodi.github.io/glow_worm_luciferin/webInterface.js'></script></body></html>)=====";

#endif //GLOW_WORM_LUCIFERIN_WEBSETTINGS_H
