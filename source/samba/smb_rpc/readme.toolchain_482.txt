==== samba
# Edit configure file line 37212 "*linux*)" to "*linux1*)"
## to prevent from running test program while cross compiling.
echo SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes >arm-linux.cache
echo samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes >>arm-linux.cache
echo samba_cv_HAVE_IFACE_IFCONF=yes >>arm-linux.cache
== neon
time CFLAGS="-march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon-vfpv4" CC=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-gcc LD=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-ld AR=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-ar ./configure --host=i686-pc-linux-gnu --target=arm-linux --prefix=/proj/mtk40123/p4ws/dtv_1401/DTV/PROD_BR/DTV_X_IDTV1401/l-pdk/device/mediatek_common/vm_linux/oss/library/gnuarm-4.8.2_neon_ca9/samba/3.0.37/ --with-ldap=no --with-cifsumount=yes --enable-swat=no --enable-cups=no --enable-static=no --with-libiconv=/proj/mtk40123/p4ws/dtv_1401/DTV/PROD_BR/DTV_X_IDTV1401/l-pdk/device/mediatek_common/vm_linux/oss/library/gnuarm-4.8.2_neon_ca9/libiconv/1.11.1 --cache-file=arm-linux.cache
== vfp_ca9
time CFLAGS="-march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp" CC=/mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-gcc LD=/mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-ld AR=/mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-ar ./configure --host=i686-pc-linux-gnu --target=arm-linux --prefix=/proj/mtk40123/p4ws/dtv_1401/DTV/PROD_BR/DTV_X_IDTV1401/l-pdk/device/mediatek_common/vm_linux/oss/library/gnuarm-4.8.2_vfp_ca9/samba/3.0.37 --with-ldap=no --with-cifsumount=yes --enable-swat=no --enable-cups=no --enable-static=no --with-libiconv=/proj/mtk40123/p4ws/dtv_1401/DTV/PROD_BR/DTV_X_IDTV1401/l-pdk/device/mediatek_common/vm_linux/oss/library/gnuarm-4.8.2_vfp_ca9/libiconv/1.11.1 --cache-file=arm-linux.cache
== rpc
= neon
time CFLAGS="-march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon-vfpv4" make -L ODB=false BUILD_CFG=rel NEW_TOOL_CHAIN=true ENABLE_CA9_NEON=true TOOL_CHAIN=4.8.2 ENABLE_CA9=false ENABLE_CA9_NEON=true
#cp -f smb_server tstatus tccp tclt ../../../library/gnuarm-4.8.2_neon_ca9/samba/3.0.37/bin/
#cp -f libsmb_mw.so libsmb_rpc.so ../../../library/gnuarm-4.8.2_neon_ca9/samba/3.0.37/lib/
= vfp_ca9
time CFLAGS="-march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp" make -L ODB=false BUILD_CFG=rel NEW_TOOL_CHAIN=true ENABLE_CA9_NEON=false TOOL_CHAIN=4.8.2 ENABLE_CA9=true ENABLE_CA9_NEON=false
#cp -f smb_server tstatus tccp tclt ../../../library/gnuarm-4.8.2_vfp_ca9/samba/3.0.37/bin/
#cp -f libsmb_mw.so libsmb_rpc.so ../../../library/gnuarm-4.8.2_vfp_ca9/samba/3.0.37/lib/

