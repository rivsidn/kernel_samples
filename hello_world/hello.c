#include <linux/kernel.h>
#include <linux/module.h>


static void fun1(void)
{
	int i;
	volatile int sum = 0;

	for(i = 0; i < 1000000; i++) {
		sum += i;
	}
}

static void fun0(void)
{
	int i;
	volatile int sum = 0;

	for(i = 0; i < 10000000; i++) {
		sum += i;
	}

	fun1();
}

static int __init demo_init(void)
{
	int i;
	printk(KERN_INFO "init\n");

	for (i = 0; i< 10000000; i++) {
		fun0();
	}

	return 0;
}
      
static void __exit demo_exit(void)
{
	printk(KERN_INFO "exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
