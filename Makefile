AS	= riscv64-unknown-elf-as
CC	= riscv64-unknown-elf-gcc
LD	= riscv64-unknown-elf-ld
CFLAGS = -fno-builtin -fno-stack-protector -fpack-struct -Wall -mcmodel=medany -mabi=lp64 -march=rv64imac -O2 -Ilibfdt
LDFLAGS = -melf64lriscv -T riscvos.lds -Map System.map

include libfdt/Makefile.libfdt

SRC_PATH	= .
BUILD_PATH  = ./obj
LIBSRCS		= lib/vsprintf.c lib/strlen.c lib/memcpy.c lib/memcmp.c lib/memchr.c lib/memmove.c lib/memset.c lib/strnlen.c lib/strrchr.c lib/strtoul.c lib/strchr.c lib/strcmp.c
EXTSRCS		= $(patsubst %.c, libfdt/%.c, $(LIBFDT_SRCS))
SRCS		= head.S trap.S main.c fdt.c proc.c sched.c vm.c global.c direct_tty.c memory.c exc.c syscall.c irq.c timer.c user.c gate.S $(LIBSRCS) $(EXTSRCS)
OBJS		= $(patsubst %.c, $(BUILD_PATH)/%.o, $(patsubst %.S, $(BUILD_PATH)/%.o, $(patsubst %.asm, $(BUILD_PATH)/%.o, $(SRCS))))
DEPS		= $(OBJS:.o=.d)

PATH := $(RISCV)/bin:$(PATH)

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
	mkdir $(BUILD_PATH)/lib
	mkdir $(BUILD_PATH)/libfdt

-include $(DEPS)

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	$(CC) $(CFLAGS) -MP -MMD -c $< -o $@

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.S
	$(CC) $(CFLAGS) -MP -MMD -c -D__ASSEMBLY__ -o $@ $<
