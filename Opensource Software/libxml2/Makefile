ifndef VM_LINUX_ROOT
VM_LINUX_ROOT := $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))
endif


PROJECT_ROOT := $(shell pwd)

export PROJECT_ROOT

.PHONY: all install

all:
	@echo 'libxml2: "make all" do nothing'
install:
	@echo 'libxml2: "make install" do nothing'
clean:
	@echo 'libxml2: "make clean" do nothing'

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/libxml2_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/libxml2_INFO.txt

