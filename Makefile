#---------------------------------------------------------------------------------
# Top-level Makefile — Trail Mix
# Builds ARM9, bundles with default ARM7 into .nds ROM
#---------------------------------------------------------------------------------

export TARGET := trailmix

# Export toolchain paths so sub-makes inherit them
export DEVKITPRO ?= /c/devkitPro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM
export LIBNDS    ?= $(DEVKITPRO)/libnds

# ARM7 binary: use local copy (checked into repo) so CI works without calico
ARM7_ELF := arm7/ds7_maine.elf

.PHONY: all arm9 clean run

all: $(TARGET).nds

# Build ARM9
arm9:
	@$(MAKE) -C arm9

# Combine ARM9 elf with default ARM7 into .nds ROM
$(TARGET).nds: arm9
	ndstool -c $(TARGET).nds \
		-9 arm9/arm9.elf \
		-7 $(ARM7_ELF) \
		-b graphics/icon.bmp "Trail Mix;Roguelike Auto-Chess;Shooter" \
		-d nitrofs

# Clean everything
clean:
	@$(MAKE) -C arm9 clean
	@rm -f $(TARGET).nds

# Run in emulator
run: $(TARGET).nds
	"c:/programmieren/dsstuff/melonDS/melonDS.exe" $(TARGET).nds
