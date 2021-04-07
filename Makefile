AS	= riscv64-unknown-elf-as
CC	= riscv64-unknown-elf-gcc
LD	= riscv64-unknown-elf-ld
CFLAGS = -fno-builtin -fno-stack-protector -Wall -mcmodel=medany -mabi=lp64 -march=rv64imac -g -Ilibfdt
LDFLAGS = -melf64lriscv -T riscvos.lds -Map System.map
LDFLAGS_USER = -melf64lriscv -nostdlib

include libfdt/Makefile.libfdt

SRC_PATH	= .
BUILD_PATH  = ./obj
LIBSRCS		= lib/vsprintf.c lib/strlen.c lib/memcpy.c lib/memcmp.c lib/memchr.c lib/memmove.c \
				lib/memset.c lib/strnlen.c lib/strrchr.c lib/strtoul.c lib/strchr.c lib/strcmp.c
EXTSRCS		= $(patsubst %.c, libfdt/%.c, $(LIBFDT_SRCS))
SRCS		= head.S trap.S main.c fdt.c proc.c sched.c vm.c global.c direct_tty.c memory.c \
				exc.c syscall.c irq.c timer.c user.c gate.S alloc.c slab.c virtio.c blk.c \
				pci.c smp.c virtio_mmio.c \
				$(LIBSRCS) $(EXTSRCS)
OBJS		= $(patsubst %.c, $(BUILD_PATH)/%.o, $(patsubst %.S, $(BUILD_PATH)/%.o, $(patsubst %.asm, $(BUILD_PATH)/%.o, $(SRCS))))

DEPS		= $(OBJS:.o=.d)

PATH := $(RISCV)/bin:$(PATH)

KERNEL	= $(BUILD_PATH)/kernel

.PHONY : everything all image run clean realclean

all : $(BUILD_PATH) $(KERNEL)
	@true

everything : $(BUILD_PATH) $(KERNEL)
	@true

image : all
	@sh gen-image.sh

run :
	@spike bbl

qemu :
	@qemu-system-riscv64 -M virt -kernel bbl -drive id=disk0,file=HD,if=none,format=raw -device virtio-blk-device,drive=disk0 -monitor stdio -bios none -device ivshmem-plain,memdev=hostmem -object memory-backend-file,size=1M,share,mem-path=/dev/shm/ivshmem,id=hostmem

runqemudbg :
	@qemu-system-riscv64 -M virt -kernel bbl -drive id=disk0,file=HD,if=none,format=raw -device virtio-blk-device,drive=disk0 -monitor stdio -bios none -s -S

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
