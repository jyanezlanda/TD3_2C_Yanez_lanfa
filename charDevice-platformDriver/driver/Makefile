KERNEL_SOURCE = /lib/modules/$(shell uname -r)/build
MOD_NAME =td3_i2c_jyl

obj-m += src/$(MOD_NAME).o


all: softclean build insmod

build:
	make -C ${KERNEL_SOURCE} M=${PWD} modules

softclean:
	make -C ${KERNEL_SOURCE} M=${PWD} clean

clean: rmmod softclean

rebuild: rmmod all

cat:
	cat /proc/modules | grep $(MOD_NAME)

insmod: 
	sudo insmod src/$(MOD_NAME).ko

rmmod:
	sudo rmmod -f src/$(MOD_NAME).ko

dmesg:
	dmesg | grep $(MOD_NAME)

awk:
	cat /proc/devices | grep $(MOD_NAME)

tail:
	tail -f /var/log/syslog

info:
	modinfo src/$(MOD_NAME).ko

dev:
	ls -l /dev/ | grep $(MOD_NAME)