#include <linux/kernel.h>
#include <linux/module.h>

static int g;

static int __init demo_init(void)
{
#if 1
	int i = 100/g;
#else
	panic("this is a panic\n");
#endif

	printk(KERN_EMERG "init %d\n", i);
	return 0;
}
      
static void __exit demo_exit(void)
{
	printk(KERN_EMERG "exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
