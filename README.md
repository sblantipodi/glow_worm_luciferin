# Glow Worm Luciferin
**Luciferin** is a generic term for the light-emitting compound found in organisms that generate bioluminescence like Fireflies and Glow Worms.
`Glow Worm Luciferin is a firmware` for ESP8266/ESP32 boards designed for the  
[Firefly Luciferin](https://github.com/sblantipodi/firefly_luciferin) software, the combination of these software create the perfect  
`Bias Lighting and Ambient Light system for PC`.  
_Written for Arduino IDE and PlatformIO._  
  
  
<img align="right" width="100" height="100" src="https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/pc_ambilight_logo.png">


[![GitHub Actions CI](https://github.com/sblantipodi/glow_worm_luciferin/actions/workflows/main.yml/badge.svg)](https://github.com/sblantipodi/glow_worm_luciferin/actions)
[![GitHub version](https://img.shields.io/github/v/release/sblantipodi/glow_worm_luciferin.svg)](https://github.com/sblantipodi/glow_worm_luciferin/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-yellow.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/sblantipodi/glow_worm_luciferin/graphs/commit-activity)
[![DPsoftware](https://img.shields.io/static/v1?label=DP&message=Software&color=orange)](https://www.dpsoftware.org)
[![Discord](https://img.shields.io/discord/747247942074892328.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/aXf9jeN)


If you like **Glow Worm Luciferin**, give it a star, or fork it and contribute!

[![GitHub stars](https://img.shields.io/github/stars/sblantipodi/pc_ambilight.svg?style=social&label=Star)](https://github.com/sblantipodi/pc_ambilight/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/sblantipodi/pc_ambilight.svg?style=social&label=Fork)](https://github.com/sblantipodi/pc_ambilight/network)
[![DPsoftware](https://img.shields.io/badge/donate-PayPal-blue)](https://www.paypal.com/donate?hosted_button_id=ZEJM8ZLQW5E4A)

Project is bootstrapped with my [Arduino Bootstrapper](https://github.com/sblantipodi/arduino_bootstrapper) library and my [PlatformIO version increment](https://github.com/sblantipodi/platformio_version_increment) script.

## Key features
- **Best in class performance** combined with ultra low CPU/GPU usage.
- Advanced algorithms for [**smooth colors transitions and color correction**](https://github.com/sblantipodi/firefly_luciferin/wiki/Color-Grading-(Hue-Saturation-and-Lightness-tuning)). Seeing is believing.
- [**Wireless or cabled, local or remote**](https://github.com/sblantipodi/firefly_luciferin/wiki/Remote-Access)? Choose your flavour with **MQTT** support and  [**Home Assistant integration**](https://github.com/sblantipodi/firefly_luciferin/wiki/Home-Automation-configs).
- [**Multi monitor**](https://github.com/sblantipodi/firefly_luciferin/wiki/Multi-monitor-support) support with **single or multiple instances**.  
- [**Web browser firmware installer**](https://sblantipodi.github.io/glow_worm_luciferin/) and [**Web Interface**](https://github.com/sblantipodi/firefly_luciferin/wiki/Remote-Access#luciferin-web-interface).  
- [**Programmable firmware**](https://github.com/sblantipodi/firefly_luciferin/wiki/Supported-GPIO-and-Baud-Rate), change your microcontroller's settings on the fly.
- Frequent updates, [**upgrade**](https://github.com/sblantipodi/firefly_luciferin/wiki/Luciferin-update-management) your PC software and your firmware **in one click**.
- Automatic [**switching between aspect ratios**](https://github.com/sblantipodi/firefly_luciferin/wiki/Aspect-ratio) based on your video content.
- **Made from a gamer, for gamers**. No added lag, stutter free.
- Multi platform, [**Windows and Linux ready**](https://github.com/sblantipodi/firefly_luciferin/wiki/Linux-support). macOS is coming when it's ready.
- If you don't want to design your own PCB and you have a soldering iron, there is [**Luciferin's official PCB**](https://github.com/sblantipodi/firefly_luciferin/wiki/Ready-to-print-PCB).
- Have a question? [**Get answered on the community**](https://discord.gg/aXf9jeN).

## Quick start
Why don't you build your own `Luciferin`?  
Here's a [**Quick Start guide**](https://github.com/sblantipodi/firefly_luciferin/wiki/Quick-start)  

## Glow Worm Luciferin + Firefly Luciferin (click to watch it on YouTube)
[![IMAGE ALT TEXT HERE](https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/pc_ambilight.png)](https://www.youtube.com/watch?v=68pnR5HMCTU)

## Components:
- ESP8266/ESP32 
- WS2812B/SK6812 5V LED Strip (60 LEDs per meter is recommended)
- 3.3V/5V Logic Level Converter 
- 220Ω resistor
- 1000uf capacitor
- Raspberry + Home Assistant for Web GUI, automations and MQTT server (optional)
- Google Home Mini for Voice Recognition (optional)

NOTE: 3.3V to 5V logic level converter is not mandatory but it is really recommended, without it, some input on the led strip digital pin could be lost. If you use a 5V microcontroller like Arduino Nano or similar you don't need it.

## Simple schematic
![CIRCUITS](https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/ambilight_bb.png)
**FULL firmware version adds support for remote control, if you flash FULL version you MUST configure** [WiFi and MQTT](https://github.com/sblantipodi/firefly_luciferin/wiki/Remote-Access). With FULL firmware USB connection is optional.  
**If you flash LIGHT firmware you MUST use USB cable**.  
Note: If you want to use the FULL firmware without using a USB cable you need to connect the 5V microcontroller's PIN to the power supply.

## Luciferin Official PCB

<img align="center" width="700" src="https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/CAD_schematics/PCB_ESP8266/img/PCB_front.jpg?raw=true">
<img align="center" width="700" src="https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/CAD_schematics/PCB_ESP8266/img/pre_after.jpg?raw=true">

If you don't want to design your own PCB and you have a soldering iron, you might find [Luciferin's official PCB](https://github.com/sblantipodi/firefly_luciferin/wiki/Ready-to-print-PCB) interesting.

## Pre-build boards support
Luciferin supports pre-build board like the [QuinLED-Dig-Uno](https://quinled.info/pre-assembled-quinled-dig-uno/).  

## Luciferin Web Interface
Glow Worm Luciferin FULL firmware exposes a `Web Interface` to control your lights from your browser without the needs of the Firefly Luciferin PC client.

<img align="center" width="750" src="https://github.com/sblantipodi/glow_worm_luciferin/blob/master/assets/img/web_interface.jpg?raw=true">

## Home Assistant Mobile Client Screenshots  
Glow Worm Luciferin firmware can be easily integrated in [Home Assistant](https://github.com/sblantipodi/firefly_luciferin/wiki/Home-Automation-configs).  
<br>
<img align="center" width="800" src="https://github.com/sblantipodi/firefly_luciferin/blob/master/data/img/ha_luciferin.jpg?raw=true">  
  
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

