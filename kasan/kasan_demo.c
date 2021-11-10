/*
 * 验证通过KASAN 定位内存越界访问问题
 *
 * 编译通过内核列表:
 * 4.4.155
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

static int type;
module_param(type, int, 0);
MODULE_PARM_DESC(type, "out bound test 0:all, 1:global, 2:global, 3: kmalloc");

static void local_out_bound(void)
{
	int local[10];

	//如果没有初始化变量，此处检测不出来
#if 1
//	memset(local, 0, sizeof(local));
#else
	for (i = 0; i < 10; i++) {
		local[i] = 1;
	}
#endif
	local[10] = 10;
	printk("%d\n", local[10]);

	return;
}

/* 可以生效 */
int global[10];
static void global_out_bound(void)
{
	global[10] = 10;
	printk("%d\n", global[10]);
}

/* 可以生效 */
static void kmalloc_out_bound(void)
{
	int *arr = kmalloc(sizeof(int)*10, GFP_KERNEL);

	arr[10] = 10;
	printk("%d\n", arr[10]);
}

static int __init demo_init(void)
{
	switch (type) {
		case 0:
			local_out_bound();
			global_out_bound();
			kmalloc_out_bound();
			break;
		case 1:
			local_out_bound();
			break;
		case 2:
			global_out_bound();
			break;
		case 3:
			kmalloc_out_bound();
			break;
		default:
			printk("invalid type\n");
			break;
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
