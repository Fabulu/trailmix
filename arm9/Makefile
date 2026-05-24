#---------------------------------------------------------------------------------
# ARM9 Makefile for Trail Mix
# Compatible with both BlocksDS and devkitPro
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
  # Try BlocksDS
  ifneq ($(strip $(BLOCKSDS)),)
    include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile
  else
    $(error "Set DEVKITARM or BLOCKSDS in your environment")
  endif
else

#---------------------------------------------------------------------------------
# devkitPro path
#---------------------------------------------------------------------------------

export LD := $(DEVKITARM)/bin/arm-none-eabi-gcc
include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# Build variables
#---------------------------------------------------------------------------------

TARGET		:= $(notdir $(CURDIR))
BUILD		:= build
SOURCES		:= source
INCLUDES	:= source data
DATA		:= data data/sprites data/enemies
GRAPHICS	:=

ARCH	:= -mthumb -mthumb-interwork -march=armv5te -mtune=arm946e-s

CFLAGS	:= -g -Wall -O2 -ffast-math \
           $(ARCH) $(INCLUDE) -DARM9

CXXFLAGS := $(CFLAGS) -std=c++17 -fno-exceptions -fno-rtti

ASFLAGS	:= -g $(ARCH)
LDFLAGS	:= -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lnds9

LIBDIRS	:= $(LIBNDS)

#---------------------------------------------------------------------------------
# Build rules (standard devkitPro template)
#---------------------------------------------------------------------------------

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:= $(CURDIR)/$(TARGET)
export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
WAVFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.wav)))

export OFILES	:= $(BINFILES:.bin=.o) $(WAVFILES:.wav=.o) \
                   $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Cleaning ARM9...
	@rm -rf $(BUILD) $(TARGET).elf

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES)

# Rule to convert .bin data files to .o via bin2s
%.o: %.bin
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)bin2s -a 4 -H $(<F:.bin=_bin.h) $< | $(AS) -o $@

# Rule to convert .wav audio files to .o via bin2s
%.o: %.wav
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)bin2s -a 4 -H $(<F:.wav=_wav.h) $< | $(AS) -o $@

-include $(DEPENDS)

endif

endif # DEVKITARM check
