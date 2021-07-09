# Building Interface Z
Here are the basic instructions to fully build the InterfaceZ. It is split into four parts:
- [Building the FPGA binary file](#building-the-fpga-binary-file)
- [Building Angular Webapp](#building-angular-webapp)
- [Building ESP32 firmware](#building-esp32-firmware)
- [Manually uploading the design](#manually-uploading-the-design)
- [Building host-mode firmware](#building-host-mode-firmware)

## Getting the source.
Make sure you have an updated repository. Place it in any path and then `export INTERFACE_Z=/path/to/interfacez`.

## Building the FPGA binary file

### Prerequisites
You will need Intel Quartus (Lite or better) in order to build the FPGA design. It will also need to include the Cyclone IV support.
You can download the Lite edition for free from [the Intel website](https://fpgasoftware.intel.com/?edition=lite).

### Building the binary bitfile (GUI)
Launch Quartus and open the project file *interfacez.qpf* . Perform a full "Compile design" step. That should generate the output file (which is
*output_files/interfacez.rbf*. No need to perform any programming of the FPGA at this point - uploading will be handled in [Manually uploading the design](#manually-uploading-the-design) .

### Building the binary bitfile (Command line)

```
cd $INTERFACE_Z/fpga/r2.2
make
cp output_files/interfacez.rbf ../../esp32/fpga.bin
```

## Building the ZX Spectrum ROM

### Prerequisites

Pasmo

TODO
  
```
cd $INTERFACE_Z/rom/
make
cp intz.rom ../esp32/spiffs/intz.rom
```

## Building Angular Webapp
### Prerequisites

- [NodeJS](https://nodejs.org/en/) 12.x or later (including `npm`)
- [Gulp](https://gulpjs.com/) `npm install --gloabal gulp-cli`

To install NodeJS on Debian/Ubuntu don't use the provided packages since they are for NodeJS  8.11

### Building the Webapp

```
cd $INTERFACE_Z/webapp
npm install
ng build --prod
gulp default
gulp install-into-spiffs
```

## Building ESP32 firmware
### Pre-requesites

- Install ESP IDF somewhere on your PC,  but outside the InterfaceZ project. For this 
project a slightly modified ESP IDF is used which is published also in github, 
based in ESP IDF release 4.2
```bash
git clone git@github.com:alvieboy/esp-idf.git
cd esp-idf
git checkout interfacez
bash install.sh
source export.sh
```

Note that anytime you need to use the IDF you need to source the "export.sh" file as above.

### Get Git Submodules

```bash
cd $INTERFACE_Z
git submodule init
git submodule update
```

### Building

Build the esp32 code by typing "make" on the esp32 directory

```bash
cd $INTERFACE_Z/esp32
make
```
It should generate all required files for programming.

## Manually uploading the design

In order to manually upload the design, you need to connect an USB cable from your PC to the debug port of Interface Z. No extra drivers should be needed.

Once you connect the device, enter programming mode using the following steps:
- Press (and DO NOT RELEASE) button IO0 on the board
- Press RESET button
- Release RESET button
- Release IO0 button.

At this point, the green LED on the top left should have stopped flashing.
Proceed to flash the InterfaceZ by issuing the following commands:
```bash
cd $INTERFACE_Z/esp32
make flash
```
It should program the board, but it will be held in programming mode. To exit programming mode back to run time mode, do
- Press RESET button

If your _Interface Z_ is connected to another port than the standard (on Linux `ttyUSB0`) use `ESPPORT` to configure it. Example:
```bash
make flash ESPPORT=/dev/ttyUSB1
```

## Building host-mode firmware
The Host-mode firmware is used solely for development.
### Prerequisites
Install these dependencies

- Debian/Ubuntu

```bash
sudo apt install libglib2.0-dev qtbase5-dev qtbase5-dev-tools qtchooser
```

- Fedora

```bash
sudo dnf install glib2-devel qt5-qtbase-devel qtchooser
```

### Building host-mode
If you already tested building the ESP32 firmware, some simple steps are required to build the host firmware:

```bash
cd $INTERFACE_Z/esp32
make -C host
```
### Starting host-mode
You can then start the application in host mode:
```bash
cd $INTERFACE_Z/esp32
host/interfacez
```
To open the web interface on host mode, point your browser to http://localhost:8000
