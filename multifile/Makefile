
obj-m := multifile.o
multifile-objs := main.o foo.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

default:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
