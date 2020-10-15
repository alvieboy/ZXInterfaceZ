all: snaloader.rom intz.rom loadscr.rom

snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384

intz.rom: pad

intz.rom: interfacez-rom.asm \
	keyboard.asm \
	string.asm \
	nmihandler.asm \
	snaram.asm      \
	snarestore.asm  \
	resource.asm    \
	debug.asm \
	port_defs.asm    \
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
