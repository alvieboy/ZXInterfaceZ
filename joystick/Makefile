
TARGET=izjoystick

all: $(TARGET).bin

CROSS=arm-none-eabi-
CC=$(CROSS)gcc
AS=$(CROSS)gcc
OBJCOPY=$(CROSS)objcopy

CUBE=.

HAL=$(CUBE)/Drivers/STM32F0xx_HAL_Driver/

HALSRCPATH=$(HAL)/Src
HALINCPATH=$(HAL)/Inc

USBDEVICE=$(CUBE)/Middlewares/ST/STM32_USB_Device_Library/
USBCOREINC=$(USBDEVICE)/Core/Inc
USBCORESRC=$(USBDEVICE)/Core/Src

USB_SOURCES=\
	usbd_core.c  \
	usbd_ctlreq.c  \
	usbd_ioreq.c

USBHIDINC=.
USBHIDSRC=.

USBHID_SOURCES=usbhid.c

HAL_SOURCES= \
stm32f0xx_hal_adc.c                 \
stm32f0xx_hal_adc_ex.c              \
stm32f0xx_hal.c                     \
stm32f0xx_hal_cortex.c              \
stm32f0xx_hal_dac.c                 \
stm32f0xx_hal_dac_ex.c  	    \
stm32f0xx_hal_dma.c                 \
stm32f0xx_hal_flash.c               \
stm32f0xx_hal_flash_ex.c            \
stm32f0xx_hal_gpio.c                \
stm32f0xx_hal_pcd.c                 \
stm32f0xx_hal_pcd_ex.c              \
stm32f0xx_hal_rcc.c                 \
stm32f0xx_hal_rcc_ex.c              \
stm32f0xx_hal_tim.c                 \
stm32f0xx_hal_tim_ex.c              \
stm32f0xx_hal_uart.c                

_HOBJ=$(HAL_SOURCES:.c=.o)
_USBOBJ=$(USB_SOURCES:.c=.o)
_USBHIDOBJ=$(USBHID_SOURCES:.c=.o)

HOBJ=$(patsubst %, $(HALSRCPATH)/%, $(_HOBJ))
USBOBJ=$(patsubst %, $(USBCORESRC)/%, $(_USBOBJ))
USBHIDOBJ=$(patsubst %, $(USBHIDSRC)/%, $(_USBHIDOBJ))

CFLAGS= -std=c11 -O2 -Wmissing-prototypes -Wall -Werror -Wno-error=unused-function \
-mcpu=cortex-m0 -I$(HALINCPATH) -I. \
-I$(CUBE)/Drivers/CMSIS/Device/ST/STM32F0xx/Include/ \
-I$(CUBE)/Drivers/CMSIS/Include \
-I$(USBCOREINC) \
-I$(USBHIDINC) \
-DUSE_HAL_DRIVER \
-DSTM32F072xB \
-fdata-sections \
-ffunction-sections 

ASFLAGS=-O2 -mcpu=cortex-m0 -x assembler-with-cpp

#XSRC=psu.c

SRC=setup.c \
    main.c \
    delay.c \
    usbd.c \
    usbd_conf.c \
    usbd_desc.c \
    ringbuffer.c \
    sbrk.c \
    cpu.c \
    debounce.c \
    joystick.c \
    timer.c \
    system_stm32f0xx.c  \
    stm32f0xx_it.c  

 
ASRC=startup_stm32f070xb.s

AOBJ=$(ASRC:.s=.o)
OBJS=$(SRC:.c=.o) $(AOBJ)



$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).elf: $(OBJS) $(HOBJ) $(USBOBJ) $(USBHIDOBJ)
#	$(CC) -o $@.dyn $+ $(CFLAGS) -shared -nostartfiles
	$(CC) $(CFLAGS) -o $@ $+ -TSTM32F072C8Tx_FLASH.ld -Wl,--gc-sections -nostartfiles -static -flto

$(AOBJ) : %.o : %.s
	$(CC) -c $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(HOBJ) $(USBOBJ) $(USBHIDOBJ) $(TARGET).elf

flash: $(TARGET).bin
	sudo dfu-util -d 0483:df11 -R -a 0 -s 0x08000000:leave -D $(TARGET).bin

