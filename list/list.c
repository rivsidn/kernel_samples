#include <linux/kernel.h>
#include <linux/module.h>

static struct list_head head;

struct TEST {
	struct list_head list;
	int value;
};

static int __init demo_init(void)
{
	struct TEST *pt;
	struct TEST test1, test2, test3; 

	memset(&test1, 0 ,sizeof(struct TEST));
	test1.value = 10;
	test2.value = 20;
	test3.value = 30;

	INIT_LIST_HEAD(&head);
	list_add_tail(&test1.list, &head);
	list_add_tail(&test2.list, &head);
	list_add_tail(&test3.list, &head);

	list_for_each_entry(pt, &head, list) {
		printk("%d\n", pt->value);
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
