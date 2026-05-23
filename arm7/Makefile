#---------------------------------------------------------------------------------
# ARM7 Makefile for DS Pill Shooter
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
  ifneq ($(strip $(BLOCKSDS)),)
    include $(BLOCKSDS)/sys/default_makefiles/rom_arm7/Makefile
  else
    $(error "Set DEVKITARM or BLOCKSDS in your environment")
  endif
else

export LD := $(DEVKITARM)/bin/arm-none-eabi-gcc
include $(DEVKITARM)/ds_rules

TARGET		:= $(notdir $(CURDIR))
BUILD		:= build
SOURCES		:= source
INCLUDES	:= source

ARCH	:= -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi

CFLAGS	:= -g -Wall -O2 $(ARCH) $(INCLUDE) -DARM7

ASFLAGS	:= -g $(ARCH)
LDFLAGS	:= -specs=ds_arm7.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lnds7

LIBDIRS	:= $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:= $(CURDIR)/$(TARGET)
export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES	:= $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Cleaning ARM7...
	@rm -rf $(BUILD) $(TARGET).elf

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif

endif
