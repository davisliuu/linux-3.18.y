cmd_arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb := arm-hisiv500-linux-gcc -E -Wp,-MD,arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.d.pre.tmp -nostdinc -I./arch/arm/boot/dts -I./arch/arm/boot/dts/include -I./drivers/of/testcase-data -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.dts.tmp arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dts ; ./scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.d.dtc.tmp arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.dts.tmp ; cat arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.d.pre.tmp arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.d.dtc.tmp > arch/arm/boot/dts/.hisi-hi3519v101-hmp-demb.dtb.d

source_arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb := arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dts

deps_arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb := \
  arch/arm/boot/dts/hisi-hi3519v101.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  arch/arm/boot/dts/include/dt-bindings/clock/hi3519-clock.h \

arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb: $(deps_arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb)

$(deps_arch/arm/boot/dts/hisi-hi3519v101-hmp-demb.dtb):
