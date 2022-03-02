#include <linux/kernel.h>
#include <linux/module.h>

static int __init demo_init(void)
{
	printk(KERN_INFO "init\n");

	//TODO: 添加livepatch 功能

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
