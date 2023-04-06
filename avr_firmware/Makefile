# Makefile
#
# targets:
#   all:          compile release version
#   debug:        compile debug version
#   test:         test the connection to the mcu
#   prog:         write compiled hex file to the mcu's flash memory
#   fuse:         write the fuse bytes to the MCU
#   eeprom_read:  read eeprom content
#   eeprom_write: write eeprom content
#   disasm:       disassemble the code for debugging
#   clean:        remove all build files

TARGET = lsmod
MCU = atmega168
CLK = 20000000

CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)

DUDE_PRG = dragon_isp
DUDE_MCU = m168
AVRDUDE = avrdude -c $(DUDE_PRG) -p $(DUDE_MCU)

# Lookup http://www.engbedded.com/fusecalc/ for fuse values
LFU = 0xFF
HFU = 0xD7
EFU = 0x01

$(info $(TARGET) firmware)

PWD := $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))

SUBDIR = $(PWD)/avr_drv
BUILD = $(PWD)/build

HEX = $(BUILD)/$(TARGET).hex
ELF = $(BUILD)/$(TARGET).elf
MAP = $(BUILD)/$(TARGET).map
EEP = $(BUILD)/$(TARGET).eep

INCS = $(wildcard $(PWD)/*.h $(foreach fd, $(SUBDIR), $(fd)/*.h))
SRCS = $(wildcard $(PWD)/*.c $(foreach fd, $(SUBDIR), $(fd)/*.c))
OBJS = $(patsubst $(PWD)/%.c, $(BUILD)/%.o, $(SRCS))

INCLUDE = -I$(PWD) $(addprefix -I, $(SUBDIR))

LDFLAGS = -lm
CFLAGS = -Wall -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE)

.PHONY: all

all: CFLAGS += -Os -fdata-sections -ffunction-sections -fomit-frame-pointer
all: $(BUILD)/$(TARGET).hex

debug: CFLAGS += -g -O0
debug: $(HEX)

$(HEX): $(ELF)
	rm -f $(HEX)
	$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(HEX)
	$(SIZE) $(ELF)

$(ELF): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$(MAP) -o $@

$(BUILD)/%.o: $(PWD)/%.c $(INCS)
	mkdir -p $(@D)
	$(CC) $< -c $(CFLAGS) $(INC_DIRS) -o $@

test:
	$(AVRDUDE) -v

prog: $(HEX)
	$(AVRDUDE) -U flash:w:$(HEX)

fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

eeprom_read:
	$(AVRDUDE) -U eeprom:r:$(EEP):i

eeprom_write: $(EEP)
	$(AVRDUDE) -U eeprom:w:$(EEP)

disasm: $(ELF)
	$(OBJDUMP) -d $(ELF)

clean:
	rm -f $(HEX) $(ELF) $(MAP) $(OBJS)
	rm -r -f build
