# OpenMC

**PROJECT IS A WORK IN PROGRESS, PIN ASSIGNMENTS MAY CHANGE**  
*Pins last changed in commit on August 15th 2021*

## So what is it ?

This project uses an ESP32 to emulate a PSX memory card. This version is not capable of save persistence. Wifi and the web interface have also been removed.

# Software setup

1. Follow this guide to setup the ESP-IDF SDK : [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)  
2. Clone this repo : `git clone https://github.com/johnbaumann/OpenMC`
3. In the 'OpenMC' directory, open a terminal and [setup environnment variables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-4-set-up-the-environment-variables)  
4. Connect your ESP32 and type `idf.py build` to compile the project.
5. Type `idf.py -p PORT [-b BAUD] flash` to upload the binary to your ESP board (Make sure you change `PORT` to your actual com port).

You can check the serial monitor to make sure the boot process is correct using `idf.py -p PORT monitor`

## Common errors

  * Certificate error at compilation time : [https://github.com/espressif/esp-idf/issues/5322#issuecomment-935331910](https://github.com/espressif/esp-idf/issues/5322#issuecomment-935331910)  
  * Message about frequency mismatch (26Mhz vs 40Mhz) :  
  Change XTAL frequency with `idf.py menuconfig` :  
  *Component config –> ESP32-specific –> Main XTAL frequency to 40 Mhz*  
  or set `CONFIG_ESP32_XTAL_FREQ_40=y` in the project's `sdkconfig` file.

# Hardware setup

## Needed hardware 

  * A PSX
  * An ESP32 board
  * Salvaged memory card or controller cable

### Connecting the ESP to the Playstation

The ESP is connected to the PSX via a memory card/pad port, either via a salvaged memory card motherboard or a butchered PSX pad cable.

#### Memory card + header

<a href="./images/mc-hdr.jpg"><img style="width:450px;" src="./images/mc-hdr.jpg"/></a>

#### PSX pad cable 

<a href="./images/pad-esp.jpg"><img style="width:450px;" src="./images/pad-esp.jpg"/></a><a href="./images/ps2-controller-pinout.png"><img style="width:450px;" src="./images/ps2-controller-pinout.png"/></a>

### ESP32-WROOM DevKit wiring

| # | PS1 Pin | ESP32 gpio |
|--------------|--------------|------------|
| 1 | DATA	|	32 |
| 2 | CMD | 34 |
| 3 | +7V | VIN |
| 4 | GND | GND |
| 5 | 3V3 | NA |
| 6 | ATT | 35 |
| 7 | CLK | 39/VN |
| 8 | ACK | 33 |

**Be mindful that ACK is pin 9 on both the memory card and pad ports, but ACK is the 8th pin of the memory card port whereas it is the 9th pin of the pad slot.**
```
_________________________
|       |       |       |
| 9 7 6 | 5 4 3 |  2 1  | CARD
|_______|_______|_______|
 _______________________
|       |       |       |
| 9 8 7 | 6 5 4 | 3 2 1 | PAD
 \______|_______|______/
```

PSX SIO pinout : [https://psx-spx.consoledev.net/pinouts/#pinouts-controller-ports-and-memory-card-ports](https://psx-spx.consoledev.net/pinouts/#pinouts-controller-ports-and-memory-card-ports)  
