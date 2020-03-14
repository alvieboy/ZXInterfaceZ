all: snaloader.rom INTZ.ROM loadscr.rom

48.ROM: zx-spectrum-rom.asm
	pasmo -d $+ $@ > $@.out
	
snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384

INTZ.ROM: menu.asm

INTZ.ROM: interfacez-rom.asm
	pasmo -d interfacez-rom.asm $@ > $@.out
	xxd -i $@ > interfacez_rom.c
	./pad $@ 16384

loadscr.rom: loadscr-rom.asm
	pasmo -d $+ $@ > $@.out
	#xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384
