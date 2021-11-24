#include <linux/kernel.h>
#include <linux/module.h>

static int __init demo_init(void)
{
	int ret;
	int a, b, c;

	/*
	 * a != b 时，a,b,c 值不变，返回值为 a 的值
	 */
	a = 1; b = 2; c = 3;
	printk("a %d b %d c %d ", a, b ,c);
	ret = cmpxchg(&a, b, c);
	printk("a %d b %d c %d ret %d\n", a, b ,c, ret);


	/*
	 * a == b 时，a 的值变成 c，b,c 值不变，返回值为 a 之前的值
	 */
	a = 2; b = 2; c = 3;
	printk("a %d b %d c %d ", a, b ,c);
	ret = cmpxchg(&a, b, c);
	printk("a %d b %d c %d ret %d\n", a, b ,c, ret);

	return 0;
}
      
static void __exit demo_exit(void)
{
	return;
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
