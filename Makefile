AS	= riscv64-unknown-elf-as
CC	= riscv64-unknown-elf-gcc
LD	= riscv64-unknown-elf-ld
CFLAGS = -fno-builtin -fno-stack-protector -fpack-struct -Wall -mcmodel=medany
LDFLAGS = -T riscvos.lds -Map System.map

SRC_PATH	= .
BUILD_PATH  = ./obj
SRCS		= head.S main.c vm.c global.c direct_tty.c
OBJS		= $(patsubst %.c, $(BUILD_PATH)/%.o, $(patsubst %.S, $(BUILD_PATH)/%.o, $(patsubst %.asm, $(BUILD_PATH)/%.o, $(SRCS))))
DEPS		= $(OBJS:.o=.d)

PATH := $(SRC_PATH)/toolchain/bin:$(PATH)

KERNEL	= $(BUILD_PATH)/kernel

.PHONY : everything all clean realclean

all : $(BUILD_PATH) $(KERNEL)
	@true

everything : $(BUILD_PATH) $(KERNEL)
	@true

clean :
	rm $(KERNEL)

realclean :
	rm $(KERNEL) $(OBJS)

$(KERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)

$(BUILD_PATH) :
	mkdir $(BUILD_PATH)

-include $(DEPS)

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	$(CC) $(CFLAGS) -MP -MMD -c $< -o $@

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.S
	$(CC) $(CFLAGS) -MP -MMD -c -D__ASSEMBLY__ -o $@ $<
