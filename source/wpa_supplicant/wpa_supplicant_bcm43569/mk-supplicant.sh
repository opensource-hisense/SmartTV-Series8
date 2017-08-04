#!/bin/bash
export PATH=$PATH:/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/
cd wpa_supplicant

if [ $1 ] && [ $1 == "clean" ]
then
    make clean    
else
    make

    cd -
#   commit_path_bin=../../../../chiling/ramdisk/hisense/hisense_rootfs/bin/
#   cp wpa_supplicant/wpa_supplicant $commit_path_bin

#   git add $commit_path_bin/wpa_supplicant
fi

