[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)]([https://opensource.org/licenses/MIT](https://github.com/th3cr34t1v3h4ck3r/RedFishShield/blob/main/LICENSE))
![](https://img.shields.io/badge/version-0.1-brightgreen)

# RedFishTools
M5Stick tools for every day hacking.

`DISCLAIMER: This is for educational purposes only and for those who are willing and curious to know and learn about Ethical Hacking, Security and Penetration Testing. Any time the word “Hacking” that is used on this site shall be regarded as Ethical Hacking. Extracting passwords from unaware victims is illegal! *** Be smart, Enjoy! ***`

![M5-Nemo Matrix Logo](https://github.com/th3cr34t1v3h4ck3r/RedFishTools/blob/main/redfishtools_app.jpg)
Logo by ivorobertis


# Features:

Orologio

	data
	ora

WiFi Scan

	6 connessioni

RedFish Portal

	172.0.0.1
 
		/creds
		/ssid
		/clear

Crypto Wallet

	9 tokens

Settings

	luminosità schermo
	aggiorna DataOra con WiFi
	reset EEPROM
	restart

Preferences (EEPROM)

	capturedCredentials
	luminosità schermo
 
# Building from Source
If you want to customize RedFishTools or contribute to the project, you should be familiar with building RedFishTools from source.
* Install Arduino IDE. I've used arduino-ide_2.3.2_Linux_64bit.AppImage on Linux and Arduino IDE Windows successfully.
* Install the M5Stack boards for Arduino IDE: In File -> Preferences, paste this URL into the "Boards Manager URLs" text box. Use commas between URLs if there are already URLs present.  https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
* If M5Stack -> M5Stick-C doesn't show up under Tools -> Boards, then use Tools -> Boards -> Boards Manager and search for M5Stack. This will install support for most of the M5Stack boards.
* Ensure the correct device model (e.g. M5Stick-C) is selected in the boards menu.
* Install necessary libraries. In Sketch -> Include Library -> Library Manager, search for and install the following libraries and any dependencies they require:
  * M5StickCPlus, M5StickC or M5Cardputer
  * IRRemoteESP8266
* Switch partition schemes. `Tools` -> `Partition Scheme` -> `No OTA (Large APP)` - sometimes this option is labeled `Huge APP` 
* Configuration
  * The code should compile cleanly and work on an M5Stick C family out of the box from the master branch or a release tag.
* Compile and upload the project

# Thanks to & Links:

	https://github.com/marivaaldo/evil-portal-m5stack/
	https://github.com/n0xa/m5stick-nemo
	https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html
	https://docs.m5stack.com/en/core/m5stickc
 	https://javl.github.io/image2cpp/



