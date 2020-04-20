# PC Ambilight
PC Ambilight based on Lightpack library   
_Written for Arduino IDE and PlatformIO._

[![GitHub version](https://img.shields.io/github/v/release/sblantipodi/pc_ambilight.svg)](https://img.shields.io/github/v/release/sblantipodi/pc_ambilight.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/sblantipodi/pc_ambilight/graphs/commit-activity)

If you like **PC Ambilight**, give it a star, or fork it and contribute!

[![GitHub stars](https://img.shields.io/github/stars/sblantipodi/pc_ambilight.svg?style=social&label=Star)](https://github.com/sblantipodi/pc_ambilight/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/sblantipodi/pc_ambilight.svg?style=social&label=Fork)](https://github.com/sblantipodi/pc_ambilight/network)

## Credits
- Davide Perini

## Components:
- Arduino C++ sketch running on an ESP8266EX D1 Mini from Lolin running @ 160MHz
- WS2812B 5V LED Strip (95 LED)
- 3.3V/5V Logic Level Converter 
- 220Ω resistor
- 1000uf capacitor for 5V power stabilization
- Raspberry + Home Assistant for Web GUI, automations and MQTT server (HA is optional but an MQTT server is needed)
- Google Home Mini for Voice Recognition (optional)

NOTE: 3.3V to 5V logic level converter is not mandatory but it is really recommended, without it, some input on the led strip digital pin could be lost. If you use a 5V microcontroller like Arduino Nano or similar you don't need it.

## Schematic
![CIRCUITS](https://github.com/sblantipodi/pc_ambilight/blob/master/data/img/ambilight_bb.png)

## Mobile Client Screenshot
![SCREENSHOT](https://github.com/sblantipodi/pc_ambilight/blob/master/data/img/HA_mobile_client_screenshot.jpg)

## PC Ambilight YouTube video (click to watch it in YouTube)
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/68pnR5HMCTU/0.jpg)](https://www.youtube.com/watch?v=68pnR5HMCTU)

## License
This program is licensed under MIT License


