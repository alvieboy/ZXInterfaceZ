all: snaloader.rom

48.ROM: zx-spectrum-rom.asm
	pasmo -d $+ $@ > $@.out
	
snaloader.rom: snaloader-rom.asm
	pasmo -d $+ $@ > $@.out
	#./pad $@ 16384
