#!/bin/sh
cp -a ../roms/intz.rom spiffs/intz.rom
LD_LIBRARY_PATH=host:host/QtSpecem exec host/gui/interfacez_gui $1 $2 $3 $4
