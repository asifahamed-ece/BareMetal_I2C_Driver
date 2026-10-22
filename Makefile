# Makefile for my STM32F4 project

# Toolchains
CROSS_COMPILE ?= arm-none-eabi-

CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE    := $(CROSS_COMPILE)size
NM      := $(CROSS_COMPILE)nm
GDB     := $(CROSS_COMPILE)gdb
OPENOCD ?= openocd

# Project layout
TARGET := app
BUILD := build
SRC_DIR := src
INC_DIR := include
TOOLS_DIR := tools

LDSCRIPT := stm32f411.ld
STARTUP := startup_stm32f411.s

# Source Enumeration
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)
ASM_SOURCES := $(STARTUP)

OBJECTS := $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o))) \
           $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
DEPS := $(OBJECTS:.o=.d)

# CPU Processor Flags
CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
WARN_FLAGS := -Wall -Wextra -Werror
OPT_FLAGS := -Og -g3 -ffunction-sections -fdata-sections
DEP_FLAGS := -MMD -MP
INCLUDES := -I$(INC_DIR)

CFLAGS := $(CPU_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(DEP_FLAGS) $(INCLUDES)
ASMFLAGS := $(CPU_FLAGS) $(OPT_FLAGS) $(DEP_FLAGS) $(INCLUDES) -x assembler-with-cpp

# -nostartfiles : we supply Reset_Handler; skip newlib's crt0.
# nano.specs    : small libc (newlib-nano).
# nosys.specs   : stub _sbrk/_write/_close/... so the link succeeds.
# --gc-sections : drop unused code/data (needs -ffunction-sections).

LDFLAGS := $(CPU_FLAGS) -nostartfiles -T$(LDSCRIPT) \
			--specs=nano.specs --specs=nosys.specs \
			-Wl,--gc-sections \
			-Wl,-Map=$(BUILD)/$(TARGET).map \
			-Wl,--print-memory-usage

# Default Targets 
.PHONY : all
all: $(BUILD)/$(TARGET).elf size

# Build Rules
$(BUILD):
	mkdir -p $(BUILD)

# C sources: .c -> .o
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Startup file gets its OWN rule: a generic `$(BUILD)/%.o: %.s` would also
# match build/main.o and try to assemble main.c as assembly. The trap is
# documented in the reference; this is the fix.
$(BUILD)/startup_stm32f411.o: $(STARTUP) | $(BUILD)
	$(CC) $(ASMFLAGS) -c $< -o $@

# Linking: .o -> .elf
$(BUILD)/$(TARGET).elf : $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@


### Convenience Targets
.PHONY: hex bin
hex: $(BUILD)/$(TARGET).hex
bin: $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

.PHONY: size
size: $(BUILD)/$(TARGET).elf
	@$(SIZE) $<

.PHONY: symbols disasm
symbols: $(BUILD)/$(TARGET).elf
	$(NM) -n -S --size-sort $<

disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d -S $<

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

# Dependency tracking

# MUST be last: .d files do not exist on the first build.
# The leading '-' makes make ignore missing includes instead of erroring.
-include $(DEPS)
