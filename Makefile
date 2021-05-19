.PHONY: clean build_kernel all user doc
all: build_kernel

U = user
K = os

TOOLPREFIX = riscv64-unknown-elf-
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
PY = python3
GDB = $(TOOLPREFIX)gdb
CP = cp
MKDIR_P = mkdir -p


BUILDDIR = build
C_SRCS = $(wildcard $K/*.c) $(wildcard $K/*/*.c)
AS_SRCS = $(wildcard $K/*.S) $(wildcard $K/*/*.S)
C_OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(C_SRCS))))
AS_OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(AS_SRCS))))

OBJS = $(C_OBJS) $(AS_OBJS)

HEADER_DEP = $(addsuffix .d, $(basename $(C_OBJS)))

ifeq (,$(findstring link_app.o,$(OBJS)))
	AS_OBJS += $(BUILDDIR)/$K/link_app.o
endif

-include $(HEADER_DEP)

ifndef CPUS
CPUS := 2
endif

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS += -MD
CFLAGS += -mcmodel=medany -mstrict-align -march=rv64imac -mabi=lp64
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I$K
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

$(AS_OBJS): $(BUILDDIR)/$K/%.o : $K/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(C_OBJS): $(BUILDDIR)/$K/%.o : $K/%.c  $(BUILDDIR)/$K/%.d
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(HEADER_DEP): $(BUILDDIR)/$K/%.d : $K/%.c
	@mkdir -p $(@D)
	@set -e; rm -f $@; $(CC) -MM $< $(INCLUDEFLAGS) > $@.$$$$; \
        sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

os/link_app.o: os/link_app.S
os/link_app.S: scripts/pack.py
	@$(PY) scripts/pack.py
os/kernel_app.ld: scripts/kernelld.py
	@$(PY) scripts/kernelld.py

build_kernel: build/kernel

build/kernel: $(OBJS) os/kernel_app.ld os/link_app.S
	$(LD) $(LDFLAGS) -T os/kernel_app.ld -o $(BUILDDIR)/kernel $(OBJS)
	$(OBJDUMP) -S $(BUILDDIR)/kernel > $(BUILDDIR)/kernel.asm
	$(OBJDUMP) -t $(BUILDDIR)/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILDDIR)/kernel.sym
	$(OBJCOPY) -O binary $(BUILDDIR)/kernel $(BUILDDIR)/ucore-kernel.bin
	@echo 'Build kernel done'

clean:
	rm -rf $(BUILDDIR) nfs/fs os/kernel_app.ld os/link_app.S

# BOARD
BOARD		?= qemu
SBI			?= rustsbi
BOOTLOADER	:= ./bootloader/fw_jump.bin

QEMU = qemu-system-riscv64
QEMUOPTS = \
	-nographic \
	-smp $(CPUS) \
	-machine virt \
	-bios $(BOOTLOADER) \
	-kernel build/kernel	\
	-drive file=$(U)/fs-copy.img,if=none,format=raw,id=x0 \
    -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0


run: build/kernel
	$(CP) $(U)/fs.img $(U)/fs-copy.img
	$(QEMU) $(QEMUOPTS)

# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::15234"; \
	else echo "-s -p 15234"; fi)

debug: build/kernel .gdbinit
	$(CP) $(U)/fs.img $(U)/fs-copy.img
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB) &
	sleep 1
	$(GDB)

user:
	make -C user

doc:
	make -C doc
