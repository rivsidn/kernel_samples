CONFIG_MODULE_SIG = n

#KVERS = $(shell uname -r)

#kernel modules
obj-m += kasan_demo.o

build: kernel_modules

kernel_modules:
	make -C /home/yuchao/workspace/blade/trunk_newkernel/platform/kernel.NEW-x86 M=$(CURDIR) modules

clean:
	make -C /home/yuchao/workspace/blade/trunk_newkernel/platform/kernel.NEW-x86 M=$(CURDIR) clean

