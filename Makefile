# PIC32MX usbhost build make file
# with PinguinoX.4 (gcc 4.6)
#

MCS=../../../../microchip_solutions_v2013-06-15
MPBASE=/Applications/microchip/xc32/v1.20
MP=$(MPBASE)/pic32mx
PINPATH=../../../pinguinoX.4-rev959
PROGDIR=../../pic32prog

# for PIC32MX220F032B
LKRSCRIPT=selfboot.ld
PROC=32MX220F032B

# for PIC32MX250F128B
#LKRSCRIPT=selfboot128.ld
#PROC=32MX250F128B

#HEAP_SIZE=512
HEAP_SIZE=2048
#HEAP_SIZE=4096

CC=$(PINPATH)/macosx/p32/bin/mips-elf-gcc
OBJC=$(PINPATH)/macosx/p32/bin/avr-objcopy
OBJDUMP=$(PINPATH)/macosx/p32/bin/mips-elf-objdump
SIZE=$(PINPATH)/macosx/p32/bin/mips-elf-size
PROG=$(PROGDIR)/pic32prog

MIPS16=-mips16

PICLIBS=$(MP)/lib/no-float/libmchp_peripheral_$(PROC).a
PROCESSOR_O=processor.o

LDFLAGS=-msoft-float -Wl,--gc-sections $(MIPS16) \
	-L. -L$(MP)/lib/proc/$(PROC)/ \
	-Wl,--defsym,_min_heap_size=$(HEAP_SIZE) \
	-Wl,-Map=output.map \
	-T$(LKRSCRIPT) \
	-T$(PINPATH)/p32/lkr/elf32pic32mx.x

ELF_FLAGS=-EL -Os -ffunction-sections -fdata-sections -march=24kc 

INCLUDEDIRS=-I. -I$(MCS)/USB -I$(MCS)/Microchip/Include -I$(MCS)/Microchip/USB \
	-I$(MP)/include -IPIC32 -IMicrochip/Include/USB/ -IBluetooth

include ./Objs.mak

CFLAGS=-fdollars-in-identifiers $(INCLUDEDIRS) -G0
CFLAGS+=-D__PIC32MX__ -D__$(PROC)__
CFLAGS+=-D__PIC32_FEATURE_SET__=200
#CFLAGS+=-DCONFIG_12MHz
#CFLAGS+=-DDEBUG_MODE
#CFLAGS+=-DUSBHOSTBT_DEBUG
CFLAGS+=-D__XC32

all: $(OBJS)
	cp $(MP)/lib/proc/$(PROC)/processor.o .
	$(CC) $(ELF_FLAGS) $(CFLAGS) -o main32.elf \
		$(PROCESSOR_O) \
		$(OBJS) \
		$(PICLIBS) \
		$(LDFLAGS) \
		-lm -lgcc -lc
	$(OBJC) -O ihex main32.elf main32.hex

%.o : %.c
	$(CC) $(ELF_FLAGS) $(CFLAGS) $(MIPS16) -c $< -o $@

crt.o : crt0.S
	$(CC) $(ELF_FLAGS) -I$(PINPATH)/p32/include/non-free -c $< -o $@

size:
	$(SIZE) main32.elf

objdump:
	$(OBJDUMP) -m mips:isa32r2 -b ihex -D main32.hex

flash:
	$(PROG) -S main32.hex

clean:
	rm -f *.o PIC32/*.o PIC32_USB/*o Bluetooth/*.o Microchip/Common/*.o \
	Microchip/USB/*.o *.elf *.hex *.map
