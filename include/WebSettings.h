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

const char settingsPage[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
<head>
    <title>LUCIFERIN Web Interface</title>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <link rel="icon" type="image/png" href="https://sblantipodi.github.io/glow_worm_luciferin/static/logos/luciferin_logo.png" >
    <script src='https://code.jquery.com/jquery-3.6.0.slim.min.js' integrity='sha256-u7e5khyithlIdTpu22PHhENmPcRdFiHRjhAuHcs05RI=' crossorigin='anonymous'></script>
    <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.1/dist/css/bootstrap.min.css' rel='stylesheet'>
    <script src='https://cdn.jsdelivr.net/npm/bootstrap@5.1.1/dist/js/bootstrap.bundle.min.js'></script>
    <script src='https://cdn.jsdelivr.net/npm/@jaames/iro@5'></script>
    <style>
        .container-fluid {
            background:orange !important;
        }
        #toggleLED {
            font-size: 1.8rem;
            font-weight: bold;
            background: lightgrey;
            border-color: lightgrey;
        }
        #toggleLED:active {
            background-color: orange !important;
            border-color: orange !important;
        }
        body {
            background-color: #f8f8f8;
            font-wight: bold;
        }
        button {
            width: 100%;
        }
        .col-sm-12, col-sm-6, footer {
            margin: 2vw 0vw 2vw 0vw;
        }
        .wrap {
            max-width: 72vw;
            margin: 0 auto;
            display: flex;
            flex-direction: row;
            align-items: center;
            justify-content: center;
        }
    </style>
</head>
<body>
<div class='container-fluid p-3 bg-primary text-white text-center'>
    <h1>LUCIFERIN</h1>
    <p>Bias Lighting and Ambient Light firmware designed for Firefly Luciferin </p>
</div>
<div class='container'>
    <div class='row'>
        <div class='col' style='margin-top: 2vw;'>
            <form onsubmit='submitForm(event)'>
                <div class='form-group row'>
                    <div class='col-sm-12'>
                        <div class='wrap'>
                            <div id='picker'></div>
                        </div>
                    </div>
                    <div class='col-sm-12'>
                        <button id='toggleLED' type='button' class='btn btn-primary' data-bs-toggle='button'
                                autocomplete='off'>Turn ON
                        </button>
                    </div>
                    <div class='col-sm-12'>
                        <div class='form-floating'>
                            <select class='form-select' id='effectSelect' aria-label='Floating label select example'>
                                <option value='solid'>Solid</option>
                                <option value='fire'>Fire</option>
                                <option value='twinkle'>Twinkle</option>
                                <option value='bpm'>Bpm</option>
                                <option value='rainbow'>Rainbow</option>
                                <option value='mixed rainbow'>Mixed rainbow</option>
                                <option value='chase rainbow'>Chase rainbow</option>
                                <option value='solid rainbow'>Solid rainbow</option>
                            </select>
                            <label for='effectSelect'>EFFECT</label>
                        </div>
                    </div>
                    <div class='col-sm-12'>
                        <div class='form-floating'>
                            <select class='form-select' id='whiteTempSelect' aria-label='Floating label select example'>
                                <option value='1'>Uncorrected temperature</option>
                                <option value='2'>1900 Kelvin</option>
                                <option value='3'>2600 Kelvin</option>
                                <option value='4'>2850 Kelvin</option>
                                <option value='5'>3200 Kelvin</option>
                                <option value='6'>5200 Kelvin</option>
                                <option value='7'>5400 Kelvin</option>
                                <option value='8'>6000 Kelvin</option>
                                <option value='9'>7000 Kelvin</option>
                                <option value='10'>20000 Kelvin</option>
                                <option value='11'>Warm Fluorescent</option>
                                <option value='12'>Standard Fluorescent</option>
                                <option value='13'>Cool White Fluorescent</option>
                                <option value='14'>Full Spectrum Fluorescent</option>
                                <option value='15'>Grow Light Fluorescent</option>
                                <option value='16'>Black Light Fluorescent</option>
                                <option value='17'>Mercury Vapor</option>
                                <option value='18'>Sodium Vapor</option>
                                <option value='19'>Metal Halide</option>
                                <option value='20'>High Pressure Sodium</option>
                            </select>
                            <label for='whiteTempSelect'>WHITE TEMPERATURE</label>
                        </div>
                    </div>
                </div>
            </form>
        </div>
    </div>
</div>
<footer class='bg-light text-center text-lg-start'>
    <div class='text-center'>
        <a class='text-dark' href='https://github.com/sblantipodi/'>Luciferin on GitHub</a>
    </div>
</footer>
<script type='text/javascript'>
    function callDevice(payload) {
        console.log(payload);
        const http = new XMLHttpRequest();
        http.open('GET', 'lights/glowwormluciferin/set?payload=' + payload);
        http.send();
        http.onload = () => console.log(http.responseText);
    }
    function createPayload() {
        var toggleLED = $('#toggleLED')[0];
        var payload = {
            state: toggleLED.classList.contains('active') ? 'ON' : 'OFF',
            effect: $('#effectSelect').val(),
            color: colorPicker.color.rgb,
            whitetemp: $('#whiteTempSelect').val()
        }
        return payload;
    }
    var colorPicker = new iro.ColorPicker('#picker', {
        width: 320,
        color: '#0091ff'
    });
    $('#effectSelect').change(function () {
        callDevice(JSON.stringify(createPayload()));
    });
    $('#whiteTempSelect').change(function () {
        callDevice(JSON.stringify(createPayload()));
    });
    $('#toggleLED').click(function () {
        callDevice(JSON.stringify(createPayload()));
        var toggleLED = $('#toggleLED')[0];
        if (toggleLED.classList.contains('active')) {
            $('#toggleLED').css('background-color','orange');
            $('#toggleLED')[0].textContent = "Turn OFF";
        } else {
            $('#toggleLED').css('background-color','lightgrey');
            $('#toggleLED')[0].textContent = "Turn ON";
        }
    });
    colorPicker.on('input:end', function(color) {
        callDevice(JSON.stringify(createPayload()));
    });
</script>
</body>
</html>)=====";

#endif //GLOW_WORM_LUCIFERIN_WEBSETTINGS_H
