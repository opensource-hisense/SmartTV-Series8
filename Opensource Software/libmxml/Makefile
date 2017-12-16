ifndef VM_LINUX_ROOT
VM_LINUX_ROOT := $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))
endif


PROJECT_ROOT := $(shell pwd)

export PROJECT_ROOT

.PHONY: all install

all:
	@echo 'libmxml: "make all" do nothing'
install:
	@echo 'libmxml: "make install" do nothing'
clean:
	@echo 'libmxml: "make clean" do nothing'

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/libmxml_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/libmxml_INFO.txt

