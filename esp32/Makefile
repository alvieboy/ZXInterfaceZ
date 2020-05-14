PROJECT_NAME := esp32_interfacez

IDF_PATH ?=/home/alvieboy/Temp/esp-idf


include $(IDF_PATH)/make/project.mk

flash: resource.bin fpga.bin

fpga.bin: ../output_files/interfacez.rbf
	@echo GEN fpga.bin
	./filesize ../output_files/interfacez.rbf fpga.bin

resource.bin: resourcelist.txt $(shell cat resourcelist.txt|cut -f2 -d' ')
	@echo GEN resource.bin
	./genresources -i resourcelist.txt -o resource.bin

ESPTOOL_ALL_FLASH_ARGS+=0x36E000 resource.bin 0x310000 fpga.bin 0x36A000 intz.rom

.PHONY: build/main/version.o
