include openbox_inc.mak


all: bash coreutils findutils gawk grep gzip inetutils module_init_tools ncurses procps sed tar util-linux-ng net-tools iputils udhcp #psmisc

bash:                                     
	cd ${BASH_SRC_PATH}; \
	tar -zxvf bash-3.2.48.tar.gz; \
	cd bash-3.2.48; \
	patch -p2 < ../bash-3.2-patches/bash32-049; \
	patch -p2 < ../bash-3.2-patches/bash32-050; \
	patch -p2 < ../bash-3.2-patches/bash32-051; \
	patch -p2 < ../bash-3.2-patches/bash32-052.txt; \
	patch -p2 < ../bash-3.2-patches/bash32-053.txt; \
	patch -p2 < ../bash-3.2-patches/bash32-054.txt

coreutils:
	cd ${COREUTIL_SRC_PATH}; \
	tar -zxvf coreutils-6.9.tar.gz; \
	cp -rf coreutils-6.9_patch/* coreutils-6.9/
	
findutils:
	cd ${FINDUTIL_SRC_PATH}; \
	tar -zxvf findutils-4.2.31.tar.gz

gawk:
	cd ${GAWK_SRC_PATH}; \
	tar -zxvf gawk-3.1.5.tar.gz

grep:
	cd ${GREP_SRC_PATH}; \
	tar -zxvf grep-2.5.1a.tar.gz; \
	cp -rf grep-2.5.1a_patch/* grep-2.5.1a/
	
	
gzip:
	cd ${GZIP_SRC_PATH}; \
	tar -zxvf gzip-1.3.12.tar.gz; \
	cp -rf gzip-1.3.12_patch/* gzip-1.3.12/
	
inetutils:
	cd ${INETUTIL_SRC_PATH}; \
	tar -zxvf inetutils-1.4.2.tar.gz; \
	cp -rf inetutils-1.4.2_patch/* inetutils-1.4.2/
	
module_init_tools:
	cd ${MODULE_INIT_SRC_PATH}; \
	tar -zxvf module-init-tools-3.12.tar.gz
	
ncurses:
	cd ${NCURSES_SRC_PATH}; \
	tar -zxvf ncurses-5.7.tar.gz
	
procps:
	cd ${PROCPS_SRC_PATH}; \
	tar -zxvf procps-3.2.8.tar.gz; \
	cp -rf procps-3.2.8_patch/* procps-3.2.8/
	
psmisc:
	cd ${PSMISC_SRC_PATH}; \
	tar -zxvf psmisc-22.13.tar.gz

sed:                                     
	cd ${SED_SRC_PATH}; \
	tar -zxvf sed-4.1.5.tar.gz; \
	cp -rf sed-4.1.5_patch/* sed-4.1.5/
	
tar:                                     
	cd ${TAR_SRC_PATH}; \
	tar -zxvf tar-1.17.tar.gz; \
	cp -rf tar-1.17_patch/* tar-1.17/

util-linux-ng:
	cd ${UTIL_LINUX_NG_SRC_PATH}; \
	tar -zxvf util-linux-ng-2.18.tar.gz; \
	cp -rf util-linux-ng-2.18_patch/* util-linux-ng-2.18/
	
net-tools:                                     
	cd ${NET_TOOLS_SRC_PATH}; \
	tar -zxvf net-tools-1.60.tar.gz; \
	cp -rf net-tools-1.60_patch/* net-tools-1.60/

iputils:                                     
	cd ${IPUTILS_SRC_PATH}; \
	tar -zxvf iputils-s20101006.tar.gz; \
	cp -rf iputils-s20101006_patch/* iputils-s20101006/

udhcp:
	cd ${UDHCP_SRC_PATH}; \
	tar -zxvf udhcp-install.tar.gz; \
	cp -rf v0.0_patch/* v0.0/

clean:
	rm -rf $(BASH_SRC_BUILD_PATH)
	rm -rf $(COREUTIL_SRC_BUILD_PATH)
	rm -rf $(FINDUTIL_SRC_BUILD_PATH)
	rm -rf $(GAWK_SRC_BUILD_PATH)
	rm -rf $(GREP_SRC_BUILD_PATH)
	rm -rf $(GZIP_SRC_BUILD_PATH)
	rm -rf $(INETUTIL_SRC_BUILD_PATH)
	rm -rf $(MODULE_INIT_SRC_BUILD_PATH)
	rm -rf $(NCURSES_SRC_BUILD_PATH)
	rm -rf $(PROCPS_SRC_BUILD_PATH)
	rm -rf $(PSMISC_SRC_BUILD_PATH)
	rm -rf $(SED_SRC_BUILD_PATH)
	rm -rf $(TAR_SRC_BUILD_PATH)
	rm -rf $(UTIL_LINUX_NG_SRC_BUILD_PATH)
	rm -rf $(NET_TOOLS_SRC_BUILD_PATH)
	rm -rf $(IPUTILS_SRC_BUILD_PATH)
	rm -rf $(UDHCP_SRC_BUILD_PATH)

