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

OBJCOPY := $(ARCH)-unknown-elf-objcopy
OBJDUMP := $(ARCH)-unknown-elf-objdump
CP := cp

elf:
	@mkdir -p build
	@cd build && cmake $(cmake_build_args) .. && make -j`proc`

bin: elf
	@mkdir -p asm
	@$(CP) build/asm/* asm
	@mkdir -p $(out_dir)
	@$(CP) -r build/target/* $(out_dir)

fs.img: bin ../nfs/fs
	../nfs/fs fs.img $(wildcard $(out_dir)/*.bin)

clean:
	@rm -rf build target asm *.img ../nfs/fs
