all:    upgrade.tar
	tar cfv upgrade.tar --transform="s/.*\///" \
	MANIFEST.json \
	../fpga.bin \
	../intz.rom \
	../build/resources.bin \
	../build//esp32_interfacez.bin
