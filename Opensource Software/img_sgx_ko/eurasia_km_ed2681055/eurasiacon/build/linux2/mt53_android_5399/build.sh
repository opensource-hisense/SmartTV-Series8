#/bin/sh
export VMLINUX=/proj/mtk70724/ws_zijie.zheng_android_20130314/DTV/DEV_BR/e_IDTV0801_LINUX_002234_jb43_merge_BR/vm_linux

export ANDROID_ROOT=$VMLINUX/android/klp-pdk

export KERNELDIR=$VMLINUX/output/mtk_android/mt5399_cn_android_KLP_PDK/debug/obj/kernel/chiling/kernel/linux-3.4/mt5399_android_smp_mod_dbg_defconfig

#export OPT_CROSS_COMPILE=$ANDROID_ROOT/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-

export OPT_CROSS_COMPILE=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-

export DISCIMAGE=$ANDROID_ROOT/device/mtk/mt5399/prebuilt/sgx

echo "DISCIMAGE=$DISCIMAGE"

echo "OPT_CROSS_COMPILE=$OPT_CROSS_COMPILE"

#export PVRSRV_DUMP_MK_TRACE=1
#export PVRSRV_USSE_EDM_STATUS_DEBUG=1

make ARCH=arm BUILD=release  PLATFORM_VERSION=4.2 SGXCORE=543 SGXCOREREV=303 SGX_FEATURE_MP=1 SGX_FEATURE_MP_CORE_COUNT=2 CROSS_COMPILE=$OPT_CROSS_COMPILE KERNEL_CROSS_COMPILE=$OPT_CROSS_COMPILE TARGET_DEVICE=mt5399
