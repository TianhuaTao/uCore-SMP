# uCore-SMP
A Symmetric Multiprocessing OS Kernel over RISC-V

## [NEW] SiFive Freedom U740 SoC

<u>**Check out fu740.md for current progress.**</u>

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

Build doc:

```shell
make doc
```

## Documentation

You can always build doc from source with `make doc`.

Also you can checkout:

[Online version](http://taotianhua.com/ucore-smp/doc)

[PDF version](http://taotianhua.com/ucore-smp/ucore-smp.pdf)

## Available Branches

- main: Fully functional OS.
- fu740: OS for HiFive Unmatched (SiFive Freedom U740 SoC)
- ch3-smp: Batch Processing OS + Time Sharing + SMP.
- ch4-smp: Batch Processing OS + Time Sharing + Virtual Memory + SMP.
- ch5-smp: Batch Processing OS + Time Sharing + Virtual Memory + Process Management + SMP.
- ch6-smp: Batch Processing OS + Time Sharing + Virtual Memory + Process Management + Pipe(Mail) + SMP.
- label-riscv: A experimental branch trying to run uCore-SMP on a specialized FPGA with data labeling.

