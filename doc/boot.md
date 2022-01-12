# 如何在 HiFive Unmatched 上启动自己的kernel

### 原理

HiFive Unmatched 默认从SD卡启动，原装的SD卡里面应该有4个分区，包含一个可以使用的Linux。

启动有5个阶段，具体可以参考 [1] https://sifive.cdn.prismic.io/sifive/05d149d5-967c-4ce3-a7b9-292e747e6582_hifive-unmatched-sw-reference-manual-v1p0.pdf 和 [2] https://sifive.cdn.prismic.io/sifive/de1491e5-077c-461d-9605-e8a0ce57337d_fu740-c000-manual-v1p3.pdf。5个阶段为：

1. ZSBL
2. U-Boot SPL
3. OpenSBI
4. U-Boot
5. Linux

最简单的方法，是把第5个阶段的Linux镜像替换掉，用我们自己的内核，然后第4阶段的U-Boot会选择我们的内核启动。**我们需要把我们的内核做成U-Boot能识别的镜像。**

观察原装的SD卡的内容，看里面内容是否和**步骤1**所列一致，不一致说明被修改过。

注：如果你的SD卡被修改过或者不是原厂的状态，可以下载一个镜像，恢复到原装的4个分区状态。下载镜像：https://github.com/sifive/freedom-u-sdk/releases/download/2021.10.00/demo-coreip-cli-unmatched-2021.10.00.rootfs.wic.xz，之后覆盖你的SD卡（替换sdX为你的SD卡设备) 内容：

```
xzcat demo-coreip-cli-unmatched-2021.10.00.rootfs.wic.xz | sudo dd of=/dev/sdX bs=512K iflag=fullblock oflag=direct conv=fsync status=progress
```

若在Windows环境下将镜像写入SD卡，我们需要使用工具rufus，具体写入方法请参考 https://sifive.cdn.prismic.io/sifive/05d149d5-967c-4ce3-a7b9-292e747e6582_hifive-unmatched-sw-reference-manual-v1p0.pdf中节2.10 FUSDK Features的内容。

这里只讨论从SD卡启动的方法，还有从PCIE SSD和FLASH启动的方法，可以参考官方文档。

### 步骤

1. 准备好原装SD卡，确保里面有4个分区，分别为

   ```
   1. primary 0 分区，Type GUID 5B193300-FC78-40CD-8002-E86C45580B47，里面装有U-Boot SPL的内容
   2. primary 1 分区，Type GUID 2E54B353-1271-4842-806F-E436D6AF6985，里面装有 OpenSBI + U-Boot + Device Tree 的内容
   3. /boot分区，FAT16格式，Linux的镜像Image.gz在这里，U-Boot会使用
   4. /root分区，ext4格式，Linux启动起来之后，根目录在这里
   
	1，2和4都不需要修改的，后续只需要动3
   ```

2. 编译你的内核。如果你没有一个现成的内核，你可以查看 https://github.com/TianhuaTao/uCore-SMP/tree/fu740 的 fu740 分支。编译的时候，要知道内核之后会被加载到 0x80200000 的DDR内存开始执行（这是SBI规定的），你的第一条内核指令就应该在这里

   在开始编译之前，我们要做一些准备工作，具体步骤如下：
   注意：请确保你的硬盘有足够大的空间（15G以上），避免由于空间不足导致的编译失败问题。

   1. 安装RISC-V编译工具链
      我们参考官方文档进行编译安装（https://github.com/riscv-collab/riscv-gnu-toolchain）

      1. 将源码下载到本地

         ```shell
         git clone https://github.com/riscv/riscv-gnu-toolchain
         ```

      2. 在Ubuntu上，我们需要先安装一些用来编译工具链的软件

         ```shell
         sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev
         ```

         若使用其他系统，请参考官方文档

      3. 在编译前进行配置

         ```shell
         ./configure --prefix=/opt/riscv --enable-multilib
         ```

         `--prefix`选项即指定安装路径

      4. 编译

         ```shell
         make
         ```

         可以通过`-j`选项并行编译，提高编译速度

         ```shell
         make -j $(nproc)
         ```

      5. 将bin目录添加到环境变量

         ```shell
         export PATH="$PATH:/opt/riscv/bin"
         ```

      6. 测试toolchain是否安装成功

         ```shell
         riscv64-unknown-elf-gcc -v
         ```

   2. 安装RISC-V QEMU
      我们参考官方文档进行编译安装（https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html）

      1. 将源码下载到本地，我们有两种方式，第一种是通过`git clone`，第二种是下载源码压缩包，解压后编译。我们为了提高速度，选择第二种方式，第一种方式可以参考官方文档进行操作。

         ```shell
         wget https://download.qemu.org/qemu-5.0.0.tar.xz
         tar xvJf qemu-5.0.0.tar.xz
         ```

      2. 在编译前进行配置

         ```shell
         ./configure --target-list=riscv64-softmmu
         ```

      3. 编译

         ```shell
         make -j $(nproc)
         ```

      4. 安装

         ```shell
         sudo make install
         ```

      5. 测试QEMU是否安装成功

         ```shell
         qemu-system-riscv64 --version
         ```

   3. 安装mkimage

      ```shell
      sudo apt-get install u-boot-tools
      ```

3. 编译完你的内核，如果它是ELF格式的，把它变成BIN格式。比如

  ```shell
  riscv64-unknown-elf-objcopy -O binary -S kernel.elf kernel.bin
  ```

4. 把内核做成U-Boot能识别的镜像。

   ```shell
   gzip -9 -cvf kernel.bin > kernel.bin.gz
   mkimage -A riscv -O linux -C gzip -T kernel -a 80200000 -e 80200000 -n "Your Name" -d kernel.bin.gz yourKernel
   ```
   yourKernel就是可以被U-Boot识别的镜像。

5. 把 yourKernel 拷贝到 SD卡 /boot的根目录下

6. 修改 /boot/extlinux/extlinux.conf 。把 
  ```
  kernel /Image.gz
  ```
  改成
  ```
  kernel /yourKernel
  ```
  现在U-Boot会自动选择 yourKernel 启动。

7. 把 SD 卡插入 HiFive Unmatched 开发板，启动。
   我们可以使用串口和开发板进行交互。
   注意：WIndows可能无法正确识别开发板，导致无法使用串口通信，这种情况需要安装驱动，具体请参考 https://sifive.cdn.prismic.io/sifive/05d149d5-967c-4ce3-a7b9-292e747e6582_hifive-unmatched-sw-reference-manual-v1p0.pdf中节3.3 FTDI Device Driver Management的内容。

### 注意事项

1. FU740不支持修改页表项中的 A 和 D，所以要手动初始化的时候都设为1。

2. 有1+4 cores (mhartid=0 是小核S7，其他是大核U7)。
3. 不⽀持 Misaligned load/store，要注意对齐。


### 参考

主页：https://www.sifive.com/boards/hifive-unmatched