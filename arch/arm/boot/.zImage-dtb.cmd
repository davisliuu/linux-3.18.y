cmd_arch/arm/boot/zImage-dtb := (cat arch/arm/boot/zImage arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb > arch/arm/boot/zImage-dtb) || (rm -f arch/arm/boot/zImage-dtb; false)
