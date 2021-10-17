/*
 * 编译成功内核列表：
 * 2.6.35.6
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>

static int value;

static int __init demo_init(void)
{
	int i;
	int *ptr = &value;
	
	printk("init...\n");

	if (IS_ERR(ptr)) {
		printk("error\n");
	} else {
		printk("not error\n");
	}

	printk("%lu\n", (unsigned long)ptr);

	for (i = 0; i < 10; i++) {
		printk("%lu\n", (unsigned long)(-i));
	}

	return 0;
}
      
static void __exit demo_exit(void)
{
	printk("exit...\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
