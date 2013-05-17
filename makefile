 
MMCU=msp430g2231
DMCU=G2231

xMCU=msp430g2211

PRG=rtc-clock

#TI_PROG=/cygdrive/C/Users/chrisc/MSP430Flasher_1.1.3/MSP430Flasher.exe
TI_PROG=/cygdrive/C/TI/MSP430Flasher_1.2.1/MSP430Flasher.exe

CC=msp430-gcc
CFLAGS=-Os -Wall -mmcu=$(MMCU) -ffunction-sections -fdata-sections -fno-inline-small-functions --cref -Wl,--relax,--section-start=.infomembcd=0x01040\

AFLAGS=-Wa,--gstabs -Wall -mmcu=$(MMCU) -x assembler-with-cpp

OBJS=$(PRG).o

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(PRG).elf $(OBJS)

size:
	msp430-size --totals $(PRG).elf

hex: $(PRG).elf
	msp430-objcopy -O ihex $(PRG).elf $(PRG).hex

lst: $(PRG).elf
	msp430-objdump -DS $(PRG).elf > $(PRG).lst

flash:
	mspdebug rf2500 "prog $(PRG).elf"
	#mspdebug rf2500 "prog $(PRG).hex"
	#$(TI_PROG) -n MSP430$(DMCU) -w "$(PRG).hex" -v -g -z [VCC]

%.o: %.S
	$(CC) $(AFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fr $(PRG).elf $(PRG).lst $(PRG).hex $(PRG).map $(PRG).txt $(OBJS)
