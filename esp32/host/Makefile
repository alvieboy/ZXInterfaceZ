all: interfacez gui/interfacez_gui

ESP_IDF=$(IDF_PATH)

FREERTOS:=./FreeRTOS
FREERTOSKERNEL:=$(FREERTOS)/Source/

GITVER=$(shell git describe --tags --dirty)
BUILDDATE=$(shell date --utc)

CFLAGS= -fpic -DPIC \
	-DGITVER=\"$(GITVER)\" \
	-DBUILDDATE='"$(BUILDDATE)"' \
	-O2 -g -Wall -Wextra -DIDF_VER=\"0.1\" \
	$(shell pkg-config --cflags glib-2.0) \
	-I$(FREERTOSKERNEL)/include \
       -I$(FREERTOSKERNEL)/portable/GCC/Linux \
       -I$(FREERTOS)/ \
       -Iesplayer \
       -Icjson \
       -I. \
       -I../main/ \
       -I$(ESP_IDF)/components/esp_system/include/ 	\
       -I$(ESP_IDF)/components/soc/soc/esp32/include/ 	\
       -I$(ESP_IDF)/components/esp_common/include/ \
       -I$(ESP_IDF)/components/spi_flash/include/ \
       -I$(ESP_IDF)/components/soc/include/ \
       -I$(ESP_IDF)/components/esp_event/include/ \
       -I$(ESP_IDF)/components/esp_wifi/include/ \
       -I$(ESP_IDF)/components/esp_timer/include/ \
       -I$(ESP_IDF)/components/fatfs/vfs/ \
       -I$(ESP_IDF)/components/fatfs/src/ \
       -I$(ESP_IDF)/components/wear_levelling/include/ \
       -I$(ESP_IDF)/components/esp_netif/include/ \
       -I$(ESP_IDF)/components/esp_eth/include/ \
       -I$(ESP_IDF)/components/spiffs/include/ \
       -I$(ESP_IDF)/components/mbedtls/mbedtls/include \
       -I$(ESP_IDF)/components/app_update/include/ \
       -I$(ESP_IDF)/components/bootloader_support/include/ \
       -I$(ESP_IDF)/components/esp_http_client/include/ \
       -I$(ESP_IDF)/components/esp_http_server/include/ \
       -I$(ESP_IDF)/components/esp_http_server/src/port/esp32 \
       -I$(ESP_IDF)/components/esp_http_server/src/util \
       -I$(ESP_IDF)/components/nghttp/port/include/ \
       -I$(ESP_IDF)/components/nvs_flash/include/ \
       -I$(ESP_IDF)/components/tcpip_adapter/include/ \
       -I$(ESP_IDF)/components/driver/include/

FREERTOS_SRC := $(FREERTOSKERNEL)/list.c \
		$(FREERTOSKERNEL)/portable/GCC/Linux/port.c \
		$(FREERTOSKERNEL)/portable/MemMang/heap_3.c \
		$(FREERTOSKERNEL)/croutine.c \
		$(FREERTOSKERNEL)/queue.c \
		$(FREERTOSKERNEL)/tasks.c \
		$(FREERTOSKERNEL)/timers.c \
		$(FREERTOSKERNEL)/event_groups.c


FREERTOS_OBJS= $(FREERTOS_SRC:.c=.o)

SRCPATH:=../main

SRC=aplist_resource.c \
config.c              \
devmap.c              \
directory_resource.c  \
dump.c                \
fileaccess.c          \
flash_pgm.c           \
flash_resource.c      \
fpga.c                \
fpga_ota.c            \
gpio.c                \
hdlc_decoder.c        \
hdlc_encoder.c        \
hid_parse.c           \
hid_usage.c           \
int8_resource.c       \
interfacez_resources.c \
interfacez_rom.c      \
keyboard.c            \
led.c                 \
list.c                \
main.c                \
netcmd.c              \
netcomms.c            \
nvs.c                 \
opstatus.c            \
opstatus_resource.c   \
ota.c                 \
res.c                 \
resource.c            \
rom.c                 \
sdcard.c              \
sna.c                 \
snaloader_rom.c       \
sna_relocs.c          \
spectcmd.c            \
spectint.c            \
spi.c                 \
status_resource.c     \
strcmpi.c             \
string_resource.c     \
tapeplayer.c          \
usb_descriptor.c      \
usb_driver.c          \
usb_driver_list.c     \
usbh.c                \
usb_hid.c             \
hid.c                 \
usb_ll.c              \
usb_mouse.c           \
version.c             \
version_resource.c    \
videostreamer.c       \
webserver.c           \
webserver_req.c       \
wifi.c                \
tzx.c                 \
tap.c                 \
json.c                \
audio.c               \
console.c             \
firmware.c            \
rle.c


_OBJ:=$(SRC:.c=.o)

OBJ:=$(patsubst %, $(SRCPATH)/%, $(_OBJ))


OBJS:=$(FREERTOS_OBJS) $(OBJ) \
	main.o \
	fpga.o \
	partition.o \
	ota.o \
	strlcpy.o \
	buttons.o \
	cjson/cJSON.o \
	cjson/cJSON_Utils.o \
	esplayer/nvs_flash.o \
	esplayer/esp_log.o \
	esplayer/driver/gpio.o \
	esplayer/driver/spi_master.o \
	$(ESP_IDF)/components/esp_http_server/src/httpd_parse.o \
	$(ESP_IDF)/components/esp_http_server/src/httpd_txrx.o  \
	$(ESP_IDF)/components/esp_http_server/src/httpd_sess.o  \
	$(ESP_IDF)/components/esp_http_server/src/httpd_ws.o    \
	$(ESP_IDF)/components/esp_http_server/src/httpd_uri.o   \
	$(ESP_IDF)/components/esp_http_server/src/httpd_main.o  \
	$(ESP_IDF)/components/esp_http_server/src/util/ctrl_sock.o  \
	$(ESP_IDF)/components/nghttp/nghttp2/third-party/http-parser/http_parser.o

interfacez: libinterfacez.so main_standalone.o
	$(CC) -o interfacez main_standalone.o libinterfacez.so

libinterfacez.so: $(OBJS) 
	$(CC) $(CFLAGS) -shared -o libinterfacez.so $(OBJS) -lpthread -lm $(shell pkg-config --libs glib-2.0)

clean:
	rm -f $(OBJS)
	
QtSpecem/Makefile:
	cd QtSpecem && qmake --qt=5 QtSpecem_lib.pro

QtSpecem/libQtSpecem.so: QtSpecem/Makefile
	make -C QtSpecem

gui/interfacez_gui: gui/Makefile libinterfacez.so QtSpecem/libQtSpecem.so
	rm -f gui/interfacez_gui
	make -C gui 
	
gui/Makefile:
	cd gui && qmake --qt=5 
