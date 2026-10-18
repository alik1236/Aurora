# Makefile — PS2 遥控小车 + 机械臂（STM32F103C8T6 标准库/寄存器直操作版）
# 用法：双击 build.bat，或先运行 build.bat 里的 PATH 配置后执行 make all

PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

CFLAGS  = -mcpu=cortex-m3 -mthumb -Os -g \
          -ffunction-sections -fdata-sections \
          -Wall -Wextra -std=gnu11 -nostdlib -ffreestanding
LDFLAGS = -T stm32f1.ld -nostdlib -Wl,--gc-sections

SRC = main.c ps2.c motor.c car.c arm.c
OBJ = $(SRC:.c=.o)

all: firmware.elf firmware.hex

firmware.elf: $(OBJ) stm32f1.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	$(SIZE) $@

firmware.hex: firmware.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OBJ) firmware.elf firmware.hex

.PHONY: all clean
