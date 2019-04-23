obj-m:=demandidle.o
KERNELDIR:=/lib/modules/$(shell uname -r)/build
#KERNELDIR:=/home/dingguangyu/c/02_src_502L04patch2/kernel/linux-4.1
PWD:=$(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.c *.mod.o *.order *.symvers
