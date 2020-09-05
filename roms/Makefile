all: snaloader.rom intz.rom loadscr.rom

snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384

intz.rom: menu.asm pad

intz.rom: interfacez-rom.asm keyboard.asm sdcardmenu.asm textinput.asm textmessage.asm wifimenu.asm string.asm \
		mainmenu.asm nmihandler.asm nmimenu.asm snaram.asm snarestore.asm resource.asm videomode.asm \
		settingsmenu.asm widget.asm debug.asm \
		port_defs.asm filechooser.asm frame.asm \
		interfacez-sysvars.asm
	pasmo -d --alocal interfacez-rom.asm $@ > $@.out
	xxd -i $@ > interfacez_rom.c
	cp $@ $@.unpad
	./pad $@ 8192

loadscr.rom: loadscr-rom.asm pad
	pasmo -d loadscr-rom.asm $@ > $@.out
	#xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384
	
pad: pad.c
	$(CC) -O2 pad.c -o pad

clean:
	@rm pad *.out interfacez_rom.c
