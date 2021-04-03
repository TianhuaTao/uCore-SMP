SRCS = $(wildcard *.S *.c)
OBJS = $(addsuffix .o, $(basename $(SRCS))) 

ifeq (,$(findstring link_app.o,$(OBJS)))
	OBJS +=link_app.o
endif

# Try to infer the correct TOOLPREFIX if not set
TOOLPREFIX = riscv64-unknown-elf-
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
PY = python3

ifndef CPUS
CPUS := 4
endif

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += -DNCPU=$(CPUS)
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096

link_app.o: link_app.S
link_app.S: pack.py
	@$(PY) pack.py
kernel_app.ld: kernelld.py
	@$(PY) kernelld.py

kernel: $(OBJS) kernel_app.ld link_app.S
	$(LD) $(LDFLAGS) -T kernel_app.ld -o kernel $(OBJS)
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

clean: 
	rm -f *.o *.d kernel *.sym *.asm link_app.S kernel_app.ld


# BOARD
BOARD		?= qemu
SBI			?= rustsbi
#BOOTLOADER	:= ../bootloader/$(SBI)-$(BOARD).bin
BOOTLOADER	:= ../bootloader/fw_jump.bin

QEMU = qemu-system-riscv64
QEMUOPTS = \
	-nographic \
	-smp $(CPUS) \
	-machine virt \
	-bios $(BOOTLOADER) \
	-kernel kernel

run: kernel
	$(QEMU) $(QEMUOPTS)


