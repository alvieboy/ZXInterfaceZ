GITVER=$(shell git describe --tags --dirty)
BUILDDATE=$(shell date --utc)

all: intz.rom loadscr.rom

snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	xxd -i $@ > snaloader_rom.c
	#./pad $@ 16384

intz.rom: pad

.PHONY: version.asm

version.asm:
	@echo "VERSION: DB \"$(GITVER) $(BUILDDATE)\",0"  > version.asm


intz.rom: interfacez-rom.asm \
	keyboard.asm \
	string.asm \
	loadpatch.asm \
	resource.asm \
	nmihandler.asm \
	snaram.asm      \
	snarestore.asm  \
	debug.asm \
	port_defs.asm    \
	z80restore.asm \
	version.asm \
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
