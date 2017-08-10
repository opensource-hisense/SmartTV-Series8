ifeq ($(CONFIG_KERNEL_UIMAGE_LZO),y)
   zreladdr-y	:= 0x00007fc0
else
   zreladdr-y	:= 0x00008000
endif
params_phys-y	:= 0x00000100
initrd_phys-y	:= 0x00800000

