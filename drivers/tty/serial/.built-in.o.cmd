cmd_drivers/tty/serial/built-in.o :=  arm-hisiv500-linux-ld -EL    -r -o drivers/tty/serial/built-in.o drivers/tty/serial/serial_core.o drivers/tty/serial/earlycon.o drivers/tty/serial/amba-pl011.o 
