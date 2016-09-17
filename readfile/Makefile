obj-m += readfile.o

#KERNELDIR ?= /lib/modules/$(shell uname -r)/build

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
