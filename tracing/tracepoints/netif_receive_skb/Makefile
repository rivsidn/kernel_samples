obj-m := tracepoint_probe.o
PWD := $(shell pwd)
KVER := $(shell uname -r)
#KDIR := /usr/src/linux-headers-`uname -r`
KDIR := /home/yuchao/workspace/blade/trunk_newkernel/platform/kernel.NEW-x86

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.mod *.mod.c *.mod.o *.ko *.symvers *.order *.a .[a-z]*

