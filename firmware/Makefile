PROJECT = ch

LOAD_ADDRESS = 0x08002000
LDSCRIPT = STM32F107xB.ld

MCU  = cortex-m3
TRGT = arm-none-eabi-

USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16 -DCORTEX_VTOR_INIT=$(LOAD_ADDRESS)
USE_LINK_GC = yes
USE_THUMB = yes
USE_VERBOSE_COMPILE = no

CHIBIOS = $(HOME)/Src/ChibiOS
include $(CHIBIOS)/os/hal/platforms/STM32F1xx/platform_f105_f107.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/ports/GCC/ARMCMx/STM32F1xx/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk
include $(CHIBIOS)/os/various/lwip_bindings/lwip.mk
include $(CHIBIOS)/test/test.mk

CSRC = $(PORTSRC) \
       $(KERNSRC) \
       $(TESTSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(LWSRC) \
       $(CHIBIOS)/os/various/evtimer.c \
       $(CHIBIOS)/os/various/shell.c \
       $(CHIBIOS)/os/various/chprintf.c \
       main.c console.c

ASMSRC = $(PORTASM)

INCDIR = $(PORTINC) $(KERNINC) $(TESTINC) \
         $(HALINC) $(PLATFORMINC) $(LWINC) \
         $(CHIBIOS)/os/various

CC   = $(TRGT)gcc
LD   = $(TRGT)gcc
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

AOPT =
TOPT = -mthumb -DTHUMB
CWARN = -Wall -Wstrict-prototypes
CPPWARN = -Wall

include $(CHIBIOS)/os/ports/GCC/ARMCMx/rules.mk

boot: build/$(PROJECT).bin
	pm -d usb1; pm -w 100 -e usb1; sleep 2
	dfu-util -d 0483:df11 -a 0 -s $(LOAD_ADDRESS):leave -D build/$(PROJECT).bin
