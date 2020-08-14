PROJECT_NAME := esp32_interfacez

IDF_PATH ?=/home/alvieboy/Temp/esp-idf

GITVER=$(shell git describe --tags --dirty)
BUILDDATE=$(shell date --utc)
EXTRA_CFLAGS+=-DGITVER=\"$(GITVER)\" -DBUILDDATE='"$(BUILDDATE)"'


include $(IDF_PATH)/make/project.mk

flash: fpga.bin

filesize: filesize.c
	$(HOSTCC) -O2 filesize.c -o filesize


fpga.bin: ../output_files/interfacez.rbf filesize
	@echo GEN fpga.bin
	./filesize ../output_files/interfacez.rbf fpga.bin

SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
SPIFFS_IMAGE_DEPENDS := 

$(eval $(call spiffs_create_partition_image,resources,spiffs))

ESPTOOL_ALL_FLASH_ARGS+=0x310000 fpga.bin 0x36A000 intz.rom


all: GENVERSION
.PHONY: GENVERSION

GENVERSION:
	$(shell touch main/version.c)
