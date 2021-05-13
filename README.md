# uCore-SMP
A Symmetric Multiprocessing OS Kernel over RISC-V

## Usage

Compile:

```shell
git clone https://github.com/TianhuaTao/uCore-SMP.git
cd uCore-SMP

# make user programs (e.g. shell)
make user

# make kernel
make
```

Run with QEMU:

```shell
make run
```

## Available Branches

- main: Fully functional OS.
- ch3-smp: Batch Processing OS + Time Sharing + SMP.
- ch4-smp: Batch Processing OS + Time Sharing + Virtual Memory + SMP.
- ch5-smp: Batch Processing OS + Time Sharing + Virtual Memory + Process Management + SMP.
- ch6-smp: Batch Processing OS + Time Sharing + Virtual Memory + Process Management + Pipe(Mail) + SMP.

