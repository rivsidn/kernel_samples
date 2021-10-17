CONFIG_MODULE_SIG = n

KVERS = $(shell uname -r)

#kernel modules
obj-m += proc_test.o

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean

