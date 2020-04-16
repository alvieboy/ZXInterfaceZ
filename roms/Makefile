all: snaloader.rom INTZ.ROM loadscr.rom

48.ROM: zx-spectrum-rom.asm
	pasmo -d $+ $@ > $@.out
	
snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384

INTZ.ROM: menu.asm pad

INTZ.ROM: interfacez-rom.asm keyboard.asm sdcardmenu.asm textinput.asm textmessage.asm wifimenu.asm string.asm
	pasmo -d --alocal interfacez-rom.asm $@ > $@.out
	xxd -i $@ > interfacez_rom.c
	./pad $@ 16384

loadscr.rom: loadscr-rom.asm pad
	pasmo -d loadscr-rom.asm $@ > $@.out
	#xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384
	
pad: pad.c
	$(CC) -O2 pad.c -o pad
