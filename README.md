# Glow Worm Luciferin
**Luciferin** is a generic term for the light-emitting compound found in organisms that generate bioluminescence like Fireflies and Glow Worms.
`Glow Worm Luciferin is a firmware` for ESP8266/ESP32 boards designed for the  
[Firefly Luciferin](https://github.com/sblantipodi/firefly_luciferin) software, the combination of these software create the perfect  
`Bias Lighting and Ambient Light system for PC`.  
_Written for Arduino IDE and PlatformIO._  
  
  
<img align="right" width="100" height="100" src="https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/pc_ambilight_logo.png">


[![GitHub Actions CI](https://github.com/sblantipodi/pc_ambilight/workflows/GitHub%20Actions%20CI/badge.svg)](https://github.com/sblantipodi/pc_ambilight/actions)
[![GitHub version](https://img.shields.io/github/v/release/sblantipodi/pc_ambilight.svg)](https://github.com/sblantipodi/pc_ambilight/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-yellow.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/sblantipodi/pc_ambilight/graphs/commit-activity)
[![DPsoftware](https://img.shields.io/static/v1?label=DP&message=Software&color=orange)](https://www.dpsoftware.org)
[![Discord](https://img.shields.io/discord/747247942074892328.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/aXf9jeN)


If you like **Glow Worm Luciferin**, give it a star, or fork it and contribute!

[![GitHub stars](https://img.shields.io/github/stars/sblantipodi/pc_ambilight.svg?style=social&label=Star)](https://github.com/sblantipodi/pc_ambilight/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/sblantipodi/pc_ambilight.svg?style=social&label=Fork)](https://github.com/sblantipodi/pc_ambilight/network)
[![DPsoftware](https://img.shields.io/badge/donate-PayPal-blue)](https://www.paypal.com/donate?hosted_button_id=ZEJM8ZLQW5E4A)

Project is bootstrapped with my [Arduino Bootstrapper](https://github.com/sblantipodi/arduino_bootstrapper) library.

## Quick start
Why don't you build your own `Luciferin`?  
Here's a [**Quick Start guide**](https://github.com/sblantipodi/firefly_luciferin/wiki/Quick-start)  

## Credits
- Davide Perini

## Components:
- Arduino C++ sketch running on an ESP8266EX/ESP32 D1 Mini from Lolin running @ 160MHz/240MHz (ESP8266 is the recommended device)
- WS2812B 5V LED Strip (60 LEDs per meter is recommended)
- 3.3V/5V Logic Level Converter 
- 220Ω resistor
- 1000uf capacitor for 5V power stabilization
- Raspberry + Home Assistant for Web GUI, automations and MQTT server (HA is optional but an MQTT server is needed)
- Google Home Mini for Voice Recognition (optional)

NOTE: 3.3V to 5V logic level converter is not mandatory but it is really recommended, without it, some input on the led strip digital pin could be lost. If you use a 5V microcontroller like Arduino Nano or similar you don't need it.

## Schematic
![CIRCUITS](https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/ambilight_bb.png)
**FULL firmware version adds support for remote control, if you flash FULL version you MUST configure** [WiFi and MQTT](https://github.com/sblantipodi/firefly_luciferin/wiki/Remote-Access). With FULL firmware USB connection is optional.  
**If you flash LIGHT firmware you MUST use USB cable**.  
Note: If you want to use the FULL firmware without using a USB cable you need to connect the 5V microcontroller's PIN to the power supply.


## Pre-build boards support
If you don't want to build your own board, you can use a pre-build board like the [QuinLED-Dig-Uno](https://quinled.info/2018/09/15/quinled-dig-uno).  
NOTE: QuinLED-Dig-Uno must be used with Glow Worm FULL Firmware because it can't be connected to your PC via USB.

## Home Assistant Mobile Client Screenshots
Glow Worm Luciferin firmware can be easily integrated in [Home Assistant](https://github.com/sblantipodi/firefly_luciferin/wiki/Home-Automation-configs).  
<img align="center" width="1000" src="https://github.com/sblantipodi/firefly_luciferin/blob/master/data/img/ha_luciferin.jpg?raw=true">  
---  
![SCREENSHOT](https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/HA_mobile_client_screenshot.jpg)

## Glow Worm Luciferin + Firefly Luciferin (click to watch it on YouTube)
[![IMAGE ALT TEXT HERE](https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/pc_ambilight.png)](https://www.youtube.com/watch?v=68pnR5HMCTU)

## Contribute
You can contribute to Luciferin by:
- Providing Pull Requests (Features, Proof of Concepts, Language files or Fixes)
- Testing new released features and report issues
- Contributing missing documentation for features and devices
- With a donation [![PayPal](https://img.shields.io/badge/donate-PayPal-blue)](https://www.paypal.com/donate?hosted_button_id=ZEJM8ZLQW5E4A)

## Thanks To 
|  Thanks              |  For                           |
|----------------------|--------------------------------|
|<a href="https://www.jetbrains.com/"><img width="200" src="https://raw.githubusercontent.com/sblantipodi/arduino_bootstrapper/master/data/img/jetbrains.png"></a>| For the <a href="https://www.jetbrains.com/clion">CLion IDE</a> licenses.|

