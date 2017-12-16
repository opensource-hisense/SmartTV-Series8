#
# image should be loaded at 
#
ifeq ($(UBOOT_VERSION), 1.3.4)
ifeq ($(PROJECT_X),y)
TEXT_BASE = 0x00d00000
else
TEXT_BASE = 0x00800000
endif

else
ifeq ($(PROJECT_X),y)
CONFIG_SYS_TEXT_BASE = 0x00d00000
else
ifneq "$(BUILTIN)" "true"  
CONFIG_SYS_TEXT_BASE = 0x01000000
else
CONFIG_SYS_TEXT_BASE = 0x03000000	#zanyun.wang add, uboot image compressed address need to rearward movement with the builtin kernel size increase
endif
endif
endif
