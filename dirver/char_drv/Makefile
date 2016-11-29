#!/bin/bash
obj-m += m_key.o
KDIR := /home/dinggy/Desktop/Project/kernel/iTop4412_Kernel_3.0
PWD ?= $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.ko *.order *.symvers *.mod.c *.mod.o *.cmd .*.o .*.ko .*.order .*.symvers .*.mod.c .*.mod.o .*.cmd
