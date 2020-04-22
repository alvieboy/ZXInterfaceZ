PROJECT_NAME := esp32_interfacez

IDF_PATH ?=/home/alvieboy/Temp/esp-idf


include $(IDF_PATH)/make/project.mk

flash: resource.bin

ESPTOOL_ALL_FLASH_ARGS+=0x36A000 resource.bin 0x310000 ../output_files/interfacez.rbf

.PHONY: build/main/version.o
