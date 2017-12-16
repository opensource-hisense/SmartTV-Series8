#! /bin/sh
#export LDFLAGS=${LDFLAGS}:-L/home/fans/test/mtp/install/lib
#echo ${LDFLAGS}
#export CPPFLAGS=-I/home/fans/test/mtp/install/include
#PKG_CONFI=pkg-config CC=arm11_mtk_le-gcc  ./configure --enable-shared --host=i686-linux --target=arm-linux --prefix=`pwd`/install
LIBUSBPATH=libusb-1.0.2
LIBUSBCOMPATPATH=libusb-compat-0.1.2
LIBMTPPATH=libmtp-0.3.6
echo $LIBUSBPATH
echo $LIBUSBCOMPATPATH
cd ..
cd ..
cd ..
cd library
cd mtp
cd mtp-0.3.6
rm usr -fr
mkdir install
cd ..
cd ..
cd ..
cd source
cd mtp
cd mtp-0.3.6
if [ ! -d ${LIBUSBPATH} ]; then
	echo "please down $LIBUSBPATH package from P4"
        exit
fi
cd $LIBUSBPATH
./build_61.sh
cd ..

if [ ! -d ${LIBUSBCOMPATPATH} ]; then
        echo "please down $LIBUSBCOMPATPATH package from P4"
        exit
fi
export PKG_CONFIG_PATH=${PWD}/libusb-1.0.2
echo $PKG_CONFIG_PATH
cd $LIBUSBCOMPATPATH
./build_61.sh
cd ..

if [ ! -d ${LIBMTPPATH} ]; then
        echo "please down $LIBMTPPATH package from P4"
        exit
fi
cd $LIBMTPPATH
./build_61.sh
cd ..

   cd ../../../library/mtp
   echo `pwd`
   cd mtp-0.3.6/
   echo `pwd`
   mv install usr
   tar -jcvf ../mtp-0.3.6-vfp-arm11-8560-install.tar.bz2 ./usr
   mv usr install
   cd ../../../source/mtp/mtp-0.3.6
   echo "MTP Install OK..."
   
   
echo "build success"
