cmd_drivers/usb/built-in.o :=  arm-hisiv500-linux-ld -EL    -r -o drivers/usb/built-in.o drivers/usb/core/built-in.o drivers/usb/host/built-in.o drivers/usb/storage/built-in.o drivers/usb/serial/built-in.o drivers/usb/misc/built-in.o drivers/usb/phy/built-in.o drivers/usb/gadget/built-in.o drivers/usb/common/built-in.o 
