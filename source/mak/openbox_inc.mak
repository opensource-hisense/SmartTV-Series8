ifndef THIS_ROOT
	THIS_ROOT=$(shell bash -c pwd -L)
endif

ifndef LINUX_ROOT
	LINUX_ROOT := $(word 1, $(subst /vm_linux/,/vm_linux /, $(THIS_ROOT)))
endif

#export ENABLE_CA9 ?= true
#export TOOL_CHAIN ?= 4.8.2

ifeq "$(TOOL_CHAIN)" "4.8.2"
    ifeq "$(ENABLE_CA9)" "true"
        export VFP_SUFFIX = _vfp_ca9
    else ifeq "$(ENABLE_CA9_NEON)" "true"
        export VFP_SUFFIX = _neon_ca9
    else ifeq "$(ENABLE_CA15_NEON)" "true"
        export VFP_SUFFIX = _neon_ca15
    endif
else ifeq "$(TOOL_CHAIN)" "4.9.1"
    ifeq "$(ENABLE_CA9_NEON)" "true"
        export VFP_SUFFIX = _neon_ca9
    endif
#else ifeq "$(TOOL_CHAIN)" "4.5.1"
#    ifeq "$(ENABLE_CA9)" "true"
#        export VFP_SUFFIX = _vfp_ca9
#    endif
endif

OSS_ROOT = $(LINUX_ROOT)/oss
OSS_SRC_ROOT = $(LINUX_ROOT)/oss/source
OSS_LIB_ROOT = $(LINUX_ROOT)/oss/library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)


include $(OSS_SRC_ROOT)/mak/oss_version.mak


export BASH_INSTALL_PATH            =   ${OSS_LIB_ROOT}/bash/${BASH_VER}
export COREUTIL_INSTALL_PATH        =   ${OSS_LIB_ROOT}/coreutils/${COREUTIL_VER}
export FINDUTIL_INSTALL_PATH        =   ${OSS_LIB_ROOT}/findutils/${FINDUTIL_VER}
export GAWK_INSTALL_PATH            =   ${OSS_LIB_ROOT}/gawk/${GAWK_VER}
export GREP_INSTALL_PATH            =   ${OSS_LIB_ROOT}/grep/${GREP_VER}
export GZIP_INSTALL_PATH            =   ${OSS_LIB_ROOT}/gzip/${GZIP_VER}
export INETUTIL_INSTALL_PATH        =   ${OSS_LIB_ROOT}/inetutils/${INETUTIL_VER}
export MODULE_INIT_INSTALL_PATH     =   ${OSS_LIB_ROOT}/module-init-tools/${MODULE_INIT_VER}
export NCURSES_INSTALL_PATH         =   ${OSS_LIB_ROOT}/ncurses/${NCURSES_VER}
export PROCPS_INSTALL_PATH          =   ${OSS_LIB_ROOT}/procps/${PROCPS_VER}
export PSMISC_INSTALL_PATH          =   ${OSS_LIB_ROOT}/psmisc/${PSMISC_VER}
export SED_INSTALL_PATH             =   ${OSS_LIB_ROOT}/sed/${SED_VER}
export TAR_INSTALL_PATH             =   ${OSS_LIB_ROOT}/tar/${TAR_VER}
export UTIL_LINUX_NG_INSTALL_PATH   =   ${OSS_LIB_ROOT}/util-linux-ng/${UTIL_LINUX_NG_VER}
export NET_TOOLS_INSTALL_PATH       =   ${OSS_LIB_ROOT}/net-tools/${NET_TOOLS_VER}
export IPUTILS_INSTALL_PATH         =   ${OSS_LIB_ROOT}/iputils/${IPUTILS_VER}
export UDHCP_INSTALL_PATH           =   ${OSS_LIB_ROOT}/udhcp/${UDHCP_VER}

export BASH_SRC_PATH                = $(OSS_SRC_ROOT)/bash
export COREUTIL_SRC_PATH            = $(OSS_SRC_ROOT)/coreutils
export FINDUTIL_SRC_PATH            = $(OSS_SRC_ROOT)/findutils
export GAWK_SRC_PATH                = $(OSS_SRC_ROOT)/gawk
export GREP_SRC_PATH                = $(OSS_SRC_ROOT)/grep
export GZIP_SRC_PATH                = $(OSS_SRC_ROOT)/gzip
export INETUTIL_SRC_PATH            = $(OSS_SRC_ROOT)/inetutils
export MODULE_INIT_SRC_PATH         = $(OSS_SRC_ROOT)/module-init-tools
export NCURSES_SRC_PATH             = $(OSS_SRC_ROOT)/ncurses
export PROCPS_SRC_PATH              = $(OSS_SRC_ROOT)/procps
export PSMISC_SRC_PATH              = $(OSS_SRC_ROOT)/psmisc
export SED_SRC_PATH                 = $(OSS_SRC_ROOT)/sed
export TAR_SRC_PATH                 = $(OSS_SRC_ROOT)/tar
export UTIL_LINUX_NG_SRC_PATH       = $(OSS_SRC_ROOT)/util-linux-ng
export NET_TOOLS_SRC_PATH           = $(OSS_SRC_ROOT)/net-tools
export IPUTILS_SRC_PATH             = $(OSS_SRC_ROOT)/iputils
export UDHCP_SRC_PATH               = $(OSS_SRC_ROOT)/udhcp


export BASH_SRC_BUILD_PATH                = $(BASH_SRC_PATH)/${BASH_VER}
export COREUTIL_SRC_BUILD_PATH            = $(COREUTIL_SRC_PATH)/${COREUTIL_VER}
export FINDUTIL_SRC_BUILD_PATH            = $(FINDUTIL_SRC_PATH)/${FINDUTIL_VER}
export GAWK_SRC_BUILD_PATH                = $(GAWK_SRC_PATH)/${GAWK_VER}
export GREP_SRC_BUILD_PATH                = $(GREP_SRC_PATH)/$(GREP_VER)
export GZIP_SRC_BUILD_PATH                = $(GZIP_SRC_PATH)/${GZIP_VER}
export INETUTIL_SRC_BUILD_PATH            = $(INETUTIL_SRC_PATH)/${INETUTIL_VER}
export MODULE_INIT_SRC_BUILD_PATH         = $(MODULE_INIT_SRC_PATH)/${MODULE_INIT_VER}
export NCURSES_SRC_BUILD_PATH             = $(NCURSES_SRC_PATH)/${NCURSES_VER}
export PROCPS_SRC_BUILD_PATH              = $(PROCPS_SRC_PATH)/${PROCPS_VER}
export PSMISC_SRC_BUILD_PATH              = $(PSMISC_SRC_PATH)/${PSMISC_VER}
export SED_SRC_BUILD_PATH                 = $(SED_SRC_PATH)/${SED_VER}
export TAR_SRC_BUILD_PATH                 = $(TAR_SRC_PATH)/${TAR_VER}
export UTIL_LINUX_NG_SRC_BUILD_PATH       = $(UTIL_LINUX_NG_SRC_PATH)/${UTIL_LINUX_NG_VER}
export NET_TOOLS_SRC_BUILD_PATH           = $(NET_TOOLS_SRC_PATH)/${NET_TOOLS_VER}
export IPUTILS_SRC_BUILD_PATH             = $(IPUTILS_SRC_PATH)/${IPUTILS_VER}
export UDHCP_SRC_BUILD_PATH               = $(UDHCP_SRC_PATH)/${UDHCP_VER}
