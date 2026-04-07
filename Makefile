#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET   := savetracker-nx
BUILD    := build
SOURCES  := src lib
DATA     := data
INCLUDES := lib
DIST     := dist

TID      := 42006D6B73740000

ARCH    := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

OPTLEVEL ?= -O2

CFLAGS  := -g -Wall -Wextra $(OPTLEVEL) -ffunction-sections -fdata-sections $(ARCH) $(DEFINES)
CFLAGS  += $(INCLUDE) -D__SWITCH__
ASFLAGS := -g $(ARCH)
LDFLAGS  = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--as-needed

LIBS    := -lnx
LIBDIRS := $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                 $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CC)

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES     := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_JSON := $(TOPDIR)/$(TARGET).json

.PHONY: $(BUILD) clean all dist

#---------------------------------------------------------------------------------
all: $(BUILD) dist

dist: $(BUILD)
	@mkdir -p $(DIST)/atmosphere/contents/$(TID)/flags
	@cp $(TARGET).nsp $(DIST)/atmosphere/contents/$(TID)/exefs.nsp
	@cp toolbox.json $(DIST)/atmosphere/contents/$(TID)/toolbox.json
	@touch $(DIST)/atmosphere/contents/$(TID)/flags/boot2.flag
	@mkdir -p $(DIST)/config/savetracker
	@cp etc/savetracker.conf.example $(DIST)/config/savetracker/savetracker.conf.example
	@cd $(DIST) && zip -q -r ../$(TARGET).zip .

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(DIST) $(TARGET).nsp $(TARGET).nso $(TARGET).npdm $(TARGET).elf $(TARGET).zip

#---------------------------------------------------------------------------------
else
.PHONY: all

DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).nsp

$(OUTPUT).nsp : $(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)
$(OFILES_SRC) : $(HFILES_BIN)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
