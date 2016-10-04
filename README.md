# Compile uboot

This is uboot fork from https://github.com/pepe2k/u-boot_mod. 

It works for GL-Domino Pi, Domino Qi and AR150.

## Download openwrt sdk
To compile the source, first you need to have a Linux machine and download openwrt/lede toolchain.

For example, you can download LEDE toolchain lede-sdk-ar71xx-generic_gcc-....bz2 from https://downloads.lede-project.org/snapshots/targets/ar71xx/generic/, or download from openwrt https://downloads.openwrt.org/chaos_calmer/15.05.1/ 

You need to read how to use the SDK here: https://wiki.openwrt.org/doc/howto/obtain.firmware.sdk 

Extract the bz2 file into one folder, e.g. /mytool/

## Compile

clone the uboot source code
```
git clone https://github.com/domino-team/uboot-domino.git uboot
cd uboot/u-boot-domino-2015
``

Now, edit Makefile

change the following line (4th line)
```
export TOOLPATH=$(BUILD_TOPDIR)/../../openwrt1407/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/
```
to
``
export TOOLPATH=$/mytool/staging_dir/toolchain-mips_24kc_gcc-5.4.0_musl-1.1.15/
``
Be sure to use our absolute path. If you download a different sdk, the path name may be different. Now type

```
make
``

After the compile finished, you should have your uboot binary in bin/uboot_for_domino.bin

# Flash it to your router



# Using the uboot

First, get a USB->UARt adapter

During the uboot boot, you need to type `gl` to stop uboot booting.




