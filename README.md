# ZX Interface Z

![Build status](https://github.com/alvieboy/ZXInterfaceZ/workflows/CI/badge.svg)

Interface Z - an expansion card for the ZX Spectrum which supports

- VGA Output
- USB devices such as joysticks, flash drives and keyboards
- Networking and Wifi
- Micro SD Card
- YM2149 audio

_Project status: Currently under development and only suitable for people that want
to help out with development and testing._

Current version is _ALPHA1_.

See [FEATURES](docs/FEATURES.md) for more details on the _ZX Interface Z_ features.
[(click here for PDF document)](docs/FEATURES.md.pdf)



# Overview

Block diagram of Interface Z [diagram](https://raw.githubusercontent.com/alvieboy/ZXInterfaceZ/master/docs/overview2.png)

# Models supported

The following models are supported:

- ZX Spectrum 16K
- ZX Spectrum 48K
- ZX Spectrum 128K (gray)
- ZX Spectrum 128K (toastrack)
- ZX Spectrum +2A
- ZX Spectrum +3


## Hardware
This section refers to the schematics and printed circuit board design of the Interface Z.
The digital design on the FPGA is described in the [FPGA design] section.
### Licensing
Schematics and PCB are released under [CC-BY-SA 2.0](https://creativecommons.org/licenses/by-sa/2.0/). This applies to all versions of the schematics and PCBs.
### Version
Current hardware version is 2.4. This is a pre-production version, not intented for general public availability.

## FPGA design
The FPGA design is written primarly in VHDL, targeting an Altera/Intel Cyclone IV device (EP4CE6E22C8N). There is no flash to store bitfile contents - the FPGA bitfile is loaded by the ESP32 MCU and stored inside the ESP32 flash.

### Licensing
Unless otherwise stated in each HDL or design support file, the FPGA design files are released under [BSD 3-Clause License](https://opensource.org/licenses/BSD-3-Clause).

## Firmware
This section applies to the firmware running on the ESP32 MCU.
### Licensing
Unless otherwise stated in each source support file, the firmware source files are released under [GPL version 3](https://opensource.org/licenses/GPL-3.0) license.

## Software
This section applies to the software intended to run on a separate computer (such as a PC, Mac or other).
### Licensing
Unless otherwise stated in each source support file, the software source files are released under [GPL version 3](https://opensource.org/licenses/GPL-3.0) license.

# Developing

See [DEVELOPING](docs/DEVELOPING.md) for development information regarding _ZX Interface Z_ 
[(click here for PDF document)](docs/DEVELOPING.md.pdf)

See [USER GUIDE](docs/USER_GUIDE.md) for build instructions.



# More information

- Follow the project on [Facebook - ZX Interface Z](https://www.facebook.com/zxinterfacez)
- Watch a [Demo of "Interface Z" for ZX Spectrum](https://www.youtube.com/watch?v=lMPc_8UKx1o) on YouTube
