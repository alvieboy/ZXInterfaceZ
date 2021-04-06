all:    upgrade.izf


RLE=./rle

./rle: rle.c
	$(CC) -O2 rle.c -o rle

intz.rle: ../intz.rom rle $(RLE)
	$(RLE) ../intz.rom intz.rle

resources.rle: ../build/resources.bin $(RLE)
	$(RLE) ../build/resources.bin resources.rle

upgrade.izf: MANIFEST.json ../fpga.bin intz.rle resources.rle ../build//esp32_interfacez.bin
	tar cfv upgrade.izf --transform="s/.*\///" \
	MANIFEST.json \
	resources.rle \
	../build//esp32_interfacez.bin

clean:
	rm upgrade.izf resources.rle
