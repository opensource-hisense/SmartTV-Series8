ifndef VM_LINUX_ROOT
VM_LINUX_ROOT := $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))
endif


PROJECT_ROOT := $(shell pwd)

export PROJECT_ROOT

.PHONY: all install clean

all:
	#make -C ./smb_rpc

clean:
	#make -C ./smb_rpc clean

install:
	#make -C ./smb_rpc install

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/samba_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/samba_INFO.txt

