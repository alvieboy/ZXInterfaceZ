PROJECT_NAME := esp32_interfacez

DEFAULTRBF=interfacez-r2.2-patch1.rbf

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
	if [ -e ../output_files/interfacez.rbf ]; then                          \
		echo Using development interfacez.rbf as FPGA firmware;        \
		$(CP) ../output_files/interfacez.rbf spiffs/fpga0.rbf;          \
	else                                                                    \
		echo Using $(DEFAULTRBF) as FPGA firmware;                      \
		$(CP) ../output_files/$(DEFAULTRBF) spiffs/fpga0.rbf;            \
	fi

SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
SPIFFS_IMAGE_DEPENDS := spiffs/fpga0.rbf spiffs/intz.rom


$(eval $(call spiffs_create_partition_image,resources,spiffs))

#ESPTOOL_ALL_FLASH_ARGS+=0x310000 fpga.bin 0x36A000 ../roms/intz.rom
.PHONY: spiffs/intz.rom

spiffs/intz.rom: ../roms/intz.rom
	$(CP) ../roms/intz.rom spiffs/intz.rom

../roms/intz.rom:
	$(MAKE) -C ../roms/ intz.rom


all: GENVERSION
.PHONY: GENVERSION

GENVERSION:
	$(shell touch main/version.c)
