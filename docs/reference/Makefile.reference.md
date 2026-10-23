# Makefile - reference answer (Phase 0.1)

Mentor's answer for the Phase 0.1 scripting assignment. Do not read this
before you have your own attempt that builds; the attempt is the exercise.

Verified on arm-none-eabi-gcc 16.2.0 / make 4.4.1:
  make                            -> app.elf, 656 B flash, 4 KB bss
  make twice                      -> second run does nothing
  touch include/*.h && make       -> exactly one recompile
  size / hex / bin / symbols / disasm / clean  -> all run

Requirement 8's trap (a greedy .s pattern rule claiming every object) is
commented in place below.

---

# BareMetal I2C Driver - STM32F411CEU6 / WeAct BlackPill V3.0
#
#   make          build (default)
#   make size     section sizes
#   make hex/bin  objcopy output
#   make flash    program via OpenOCD
#   make debug    OpenOCD + GDB, breakpoint at main
#   make clean    remove build/
#
# Recipe lines MUST be indented with a TAB.

CROSS_COMPILE ?= arm-none-eabi-
CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE    := $(CROSS_COMPILE)size
NM      := $(CROSS_COMPILE)nm
GDB     := $(CROSS_COMPILE)gdb
OPENOCD ?= openocd

TARGET    := app
BUILD     := build
SRC_DIR   := src
INC_DIR   := include
TOOLS_DIR := tools

LDSCRIPT := stm32f411.ld
STARTUP  := startup_stm32f411.s

C_SOURCES   := $(wildcard $(SRC_DIR)/*.c)
ASM_SOURCES := $(STARTUP)

OBJECTS := $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o))) \
           $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
DEPS    := $(OBJECTS:.o=.d)

CPU_FLAGS  := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
WARN_FLAGS := -Wall -Wextra -Werror
OPT_FLAGS  := -Og -g3 -ffunction-sections -fdata-sections
DEP_FLAGS  := -MMD -MP
INCLUDES   := -I$(INC_DIR)

CFLAGS  := $(CPU_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(DEP_FLAGS) $(INCLUDES)
ASFLAGS := $(CPU_FLAGS) $(OPT_FLAGS) $(INCLUDES) -x assembler-with-cpp

# -nostartfiles: we supply Reset_Handler ourselves. nosys.specs: no OS.
LDFLAGS := $(CPU_FLAGS) -nostartfiles -T $(LDSCRIPT) \
           --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD)/$(TARGET).map \
           -Wl,--print-memory-usage

.PHONY: all
all: $(BUILD)/$(TARGET).elf size

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Static pattern rule: a generic "$(BUILD)/%.o: $(STARTUP)" would also claim
# build/main.o and assemble main.o out of the startup file.
$(BUILD)/startup_stm32f411.o: $(STARTUP) | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.PHONY: hex bin
hex: $(BUILD)/$(TARGET).hex
bin: $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# text+data = flash, data+bss = RAM
.PHONY: size
size: $(BUILD)/$(TARGET).elf
	@$(SIZE) $<

.PHONY: disasm symbols
disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d -S $<

symbols: $(BUILD)/$(TARGET).elf
	$(NM) -n -S --size-sort $<

.PHONY: flash
flash: $(BUILD)/$(TARGET).elf
	$(OPENOCD) -f $(TOOLS_DIR)/openocd.cfg \
	    -c "program $< verify reset exit"

.PHONY: debug
debug: $(BUILD)/$(TARGET).elf
	$(OPENOCD) -f $(TOOLS_DIR)/openocd.cfg &
	$(GDB) -ex "target extended-remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load $<" \
	       -ex "break main" \
	       $<

.PHONY: clean
clean:
	rm -rf $(BUILD)

# Must be last: the .d files do not exist until the first build.
-include $(DEPS)
