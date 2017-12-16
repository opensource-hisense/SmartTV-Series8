#/bin/sh
export VMLINUX=/proj/mtk70724/ws_zijie.zheng_android_20120511/DTV/PROD_BR/DTV_X_IDTV0801/vm_linux

export ANDROID_ROOT=$VMLINUX/android/jb-4.x

export KERNELDIR=$VMLINUX/output/mtk_android/mt5399_cn_android_JB/debug/obj/kernel/chiling/kernel/linux-3.4/mt5399_android_smp_mod_dbg_defconfig

export OPT_CROSS_COMPILE=$ANDROID_ROOT/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-

export DISCIMAGE=$ANDROID_ROOT/device/mtk/mt5396/prebuilt/sgx

echo "DISCIMAGE=$DISCIMAGE"

echo "OPT_CROSS_COMPILE=$OPT_CROSS_COMPILE"
make install ARCH=arm BUILD=debug  PLATFORM_VERSION=4.2 SGXCORE=543 SGXCOREREV=303 SGX_FEATURE_MP=1 SGX_FEATURE_MP_CORE_COUNT=2 CROSS_COMPILE=$OPT_CROSS_COMPILE KERNEL_CROSS_COMPILE=$OPT_CROSS_COMPILE TARGET_DEVICE=mt5399
