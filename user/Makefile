.DEFAULT_GOAL := bin

ARCH ?= riscv64
MODE ?= release
out_dir := target/bin
cmake_build_args := -DARCH=$(ARCH)
ifeq ($(MODE), release)
cmake_build_args += -DCMAKE_BUILD_TYPE=Release
else ifeq ($(MODE), debug)
cmake_build_args += -DCMAKE_BUILD_TYPE=Debug
endif

OBJCOPY := $(ARCH)-linux-musl-objcopy
OBJDUMP := $(ARCH)-linux-musl-objdump
CP := cp
PY := python3

elf:
	@mkdir -p build
	@cd build && cmake $(cmake_build_args) .. && make -j

ch3-elf:


bin: elf
	@mkdir -p asm
	@$(CP) build/asm/* asm
	@mkdir -p $(out_dir)
	@$(CP) build/bin/* $(out_dir)

clean:
	@rm -rf build target asm