CONFIG_MODULE_SIG = n

KVERS = $(shell uname -r)

#kernel modules
obj-m += protocol.o

build: kernel_modules

kernel_modules:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64-clean M=$(CURDIR) modules
#	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) modules
#	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64-clean M=$(CURDIR) clean
#	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) clean
#	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean

