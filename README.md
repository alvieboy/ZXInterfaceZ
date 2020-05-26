# ZX Interface Z

Interface Z - an expansion card for the ZX Spectrum which supports

- VGA Output
- USB Support
- Wifi
- Micro SD Card

_Project status: Currently this is in early development and only suitable for people that want to help out with development and testing._

# Overview

Block diagram of Interface Z [diagram](https://raw.githubusercontent.com/alvieboy/ZXInterfaceZ/master/docs/overview.png)

## Hardware
This section refers to the schematics and printed circuit board design of the Interface Z. The digital design on the FPGA is described in the [FPGA design] section.
### Licensing
Schematics and PCB are released under [CC-BY-SA 2.0](https://creativecommons.org/licenses/by-sa/2.0/). This applies to all versions of the schematics and PCBs.
### Version
Current hardware version is 2.1. This is a prototype version, not intented for general public availability.
### Status (version 2.1)
- Interfacing with ZX Spectrum 48K proven.
- Support of other Spectrums, like 128K, +2A, +3, will need at least #1 fixed.
- IRQn request still to be tested
- USB interface proven at physical level with Full-Speed and Low-Speed devices, with minor hardware patch (#3)
- SD Card working, but might need update for production due to conflicting pullups (#1).

## FPGA design
The FPGA design is written primarly in VHDL, targeting an Altera/Intel Cyclone IV device (EP4CE6E22C8N). There is no flash to store bitfile contents - the FPGA bitfile is loaded by the ESP32 MCU and stored inside the ESP32 flash.

### Licensing
Unless otherwise stated in each HDL or design support file, the FPGA design files are released under [BSD 3-Clause License](https://opensource.org/licenses/BSD-3-Clause).


TODO
## Firmware
This section applies to the firmware running on the ESP32 MCU.
### Licensing
Unless otherwise stated in each source support file, the firmware source files are released under [GPL version 3](https://opensource.org/licenses/GPL-3.0) license.


TODO

## Software
This section applies to the software intended to run on a separate computer (such as a PC, Mac or other).
### Licensing
Unless otherwise stated in each source support file, the software source files are released under [GPL version 3](https://opensource.org/licenses/GPL-3.0) license.

# More information

- Follow the project on [Facebook - ZX Interface Z](https://www.facebook.com/zxinterfacez)
- Watch a [Demo of "Interface Z" for ZX Spectrum](https://www.youtube.com/watch?v=lMPc_8UKx1o) on YouTube
