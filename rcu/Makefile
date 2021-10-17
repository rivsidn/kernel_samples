CONFIG_MODULE_SIG = n

#KVERS = $(shell uname -r)

#kernel modules
obj-m += rcu_demo.o

build: kernel_modules

kernel_modules:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) modules
#	make -C /home/yuchao/workspace/blade/trunk_newkernel/platform/kernel.NEW-x86 M=$(CURDIR) modules
#	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /home/yuchao/workspace/blade/main_longtime/platform/kernel.x86_64 M=$(CURDIR) clean
#	make -C /home/yuchao/workspace/blade/trunk_newkernel/platform/kernel.NEW-x86 M=$(CURDIR) clean
#	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean

