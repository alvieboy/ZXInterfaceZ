PROJECT_NAME := esp32_interfacez

BOARDREV:=r2.2

RBFLOC=../fpga/$(BOARDREV)/output_files/
DEFAULTRBF=interfacez-$(BOARDREV)-patch1.rbf

IDF_PATH ?=/home/alvieboy/Temp/esp-idf
CP:=cp

GITVER=$(shell git describe --tags --dirty)
BUILDDATE=$(shell date --utc)
EXTRA_CFLAGS+=-DGITVER=\"$(GITVER)\" -DBUILDDATE='"$(BUILDDATE)"'

EXTRA_COMPONENT_DIRS=main/wsys main/menus

include $(IDF_PATH)/make/project.mk

flash: spiffs/fpga0.rbf spiffs/intz.rom

#filesize: filesize.c
#	$(HOSTCC) -O2 filesize.c -o filesize


.PHONY: spiffs/fpga0.rbf
spiffs/fpga0.rbf:
	if [ -e $(RBFLOC)/interfacez.rbf ]; then                          \
		echo Using development $(RBFLOC)/interfacez.rbf as FPGA firmware;        \
		$(CP) $(RBFLOC)/interfacez.rbf spiffs/fpga0.rbf;          \
	else                                                                    \
		echo Using $(RBFLOC)/$(DEFAULTRBF) as FPGA firmware;                      \
		$(CP) $(RBFLOC)/$(DEFAULTRBF) spiffs/fpga0.rbf;            \
	fi

SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
SPIFFS_IMAGE_DEPENDS := spiffs/fpga0.rbf spiffs/intz.rom


$(eval $(call spiffs_create_partition_image,resources,spiffs))

#ESPTOOL_ALL_FLASH_ARGS+=0x310000 fpga.bin 0x36A000 ../roms/intz.rom
.PHONY: spiffs/intz.rom

spiffs/intz.rom: ../roms/intz.rom
	@echo Copying ROM into spifss/intz.rom
	$(CP) ../roms/intz.rom spiffs/intz.rom

../roms/intz.rom:
	CC=gcc $(MAKE) -C ../roms/ intz.rom


all: GENVERSION
.PHONY: GENVERSION

GENVERSION:
	$(shell touch main/version.c)

#$(eval $(call spiffs_create_partition_image,resources,spiffs))

rle:	rle.c rle.h
	gcc -O2 -o rle rle.c

release: all $(BUILD_DIR_BASE)/resources.bin rle
	mkdir -p release
	$(PYTHON) genfirmware.py --version=$(GITVER) --compat=$(BOARDREV) --out=release/interfacez_firmware_$(BOARDREV)_$(GITVER)_$(BUILDDATE).izf
