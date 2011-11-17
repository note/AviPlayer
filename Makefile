#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!    
#
##############################################################################################
# 
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#

##############################################################################################
# Start of default section
#

TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
BIN  = $(CP) -O binary 

# MCU core
MCU = arm926ej-s

# MCU model
SUBMDL = AT91SAM9260

LDFLAGS +=-Tinclude/sdram.lds
LDFLAGS +=-Tinclude/sram.lds

# List all default C defines here, like -D_DEBUG=1
DDEFS = 

# List all default ASM defines here, like -D_DEBUG=1
DADEFS = 

# List all default directories to look for include files here
DINCDIR = 

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS = 

#
# End of default section
##############################################################################################

##############################################################################################
# Start of user section
#

# Define project name here
PROJECT = test

# Define linker script file here
LDSCRIPT_RAM = ./include/sdram.lds
LDSCRIPT_ROM = ./include/sdram.lds

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS = 

#FAT Filesystem
FAT_SRC = fat/ff.c fat/diskio.c fat/time.c \
fat/option/ccsbcs.c

# List C source files here
SRC  =  main.c Cstartup_SAM9.c syscalls.c delay.c dbgu.c lcd.c term_io.c sd.c $(FAT_SRC) avi.c

# List ASM source files here
ASRC = cstartup.s

# List all user directories here
UINCDIR = ./include

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS = 

# Define optimisation level here
OPT = -O0

#
# End of user defines
##############################################################################################


INCDIR  = $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))
LIBDIR  = $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))
DEFS    = $(DDEFS) $(UDEFS)
ADEFS   = $(DADEFS) $(UADEFS)
OBJS    = $(ASRC:.s=.o) $(SRC:.c=.o)
LIBS    = $(DLIBS) $(ULIBS)

# In case THUMB code should be created, add "-mthumb" to the MCFLAGS too.
MCFLAGS = -mcpu=$(MCU)

ASFLAGS = $(MCFLAGS) -g -gdwarf-2 -Wa,-amhls=$(<:.s=.lst) $(ADEFS)
CPFLAGS = $(MCFLAGS) $(OPT) -gdwarf-2 -mthumb-interwork -fomit-frame-pointer -Wall -Wstrict-prototypes -fverbose-asm -Wa,-ahlms=$(<:.c=.lst) $(DEFS)
CONLYFLAGS = $(CSTANDARD)
LDFLAGS_RAM = $(MCFLAGS) -nostartfiles -T$(LDSCRIPT_RAM) -Wl,-Map=$(PROJECT)_ram.map,--cref,--no-warn-mismatch $(LIBDIR)
LDFLAGS_ROM = $(MCFLAGS) -nostartfiles -T$(LDSCRIPT_ROM) -Wl,-Map=$(PROJECT)_rom.map,--cref,--no-warn-mismatch $(LIBDIR)

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Generate dependency information
CPFLAGS += -MD -MP -MF .dep/$(@F).d

#
# makefile rules
#

all: RAM ROM

RAM: $(OBJS) $(PROJECT)_ram.elf $(PROJECT)_ram.bin

ROM: $(OBJS) $(PROJECT)_rom.elf $(PROJECT)_rom.bin

%.o : %.c
	$(CC) -c $(CPFLAGS) $(CONLYFLAGS) -I . $(INCDIR) $< -o $@

%.o : %.s
	$(AS) -c $(ASFLAGS) $< -o $@

%ram.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_RAM) $(LIBS) -o $@

%rom.elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_ROM) $(LIBS) -o $@

%bin: %elf
	$(BIN) $< $@
	


clean:
	-rm -f $(OBJS)
	-rm -f $(PROJECT)_ram.elf
	-rm -f $(PROJECT)_ram.map
	-rm -f $(PROJECT)_ram.bin
	-rm -f $(PROJECT)_rom.elf
	-rm -f $(PROJECT)_rom.map
	-rm -f $(PROJECT)_rom.bin
	-rm -f $(SRC:.c=.c.bak)
	-rm -f $(SRC:.c=.lst)
	-rm -fR .dep


#	-rm -f $(ASRC:.s=.s.bak)
#	-rm -f $(ASRC:.s=.lst)

# 
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***