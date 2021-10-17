#include <linux/kernel.h>
#include <linux/module.h>

static int __init demo_init(void)
{
	printk(KERN_EMERG "init\n");

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
