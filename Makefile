#---------------------------------------------------------------------------------
# Top-level Makefile — DS Pill Shooter
# Builds ARM9, bundles with default ARM7 into .nds ROM
#---------------------------------------------------------------------------------

export TARGET := dsgame

# Export toolchain paths so sub-makes inherit them
export DEVKITPRO ?= /c/devkitPro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM
export LIBNDS    ?= $(DEVKITPRO)/libnds

CALICO := $(DEVKITPRO)/calico

.PHONY: all arm9 clean run

all: $(TARGET).nds

# Build ARM9
arm9:
	@$(MAKE) -C arm9

# Combine ARM9 elf with default ARM7 into .nds ROM
$(TARGET).nds: arm9
	ndstool -c $(TARGET).nds \
		-9 arm9/arm9.elf \
		-7 $(CALICO)/bin/ds7_maine.elf \
		-b graphics/icon.bmp "Pill Shooter;DS Roguelike;Homebrew"

# Clean everything
clean:
	@$(MAKE) -C arm9 clean
	@rm -f $(TARGET).nds

# Run in emulator
run: $(TARGET).nds
	"c:/programmieren/dsstuff/melonDS/melonDS.exe" $(TARGET).nds
