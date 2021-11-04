CONFIG_MODULE_SIG = n

#KVERS = $(shell uname -r)

#kernel modules
obj-m += lockdep_test.o

build: kernel_modules

kernel_modules:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) modules

clean:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) clean

