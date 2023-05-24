#include <linux/kernel.h>
#include <linux/module.h>

static struct list_head head;

struct TEST {
	struct list_head list;
	int value;
};

static int __init demo_init(void)
{
	int i = 0;
	struct TEST *tmp, *test;
	struct TEST test1;

	memset(&test1, 0 ,sizeof(struct TEST));
	test1.value = 10;

	INIT_LIST_HEAD(&head);
	list_add(&test1.list, &head);
	list_add(&test1.list, &head);

	list_for_each_entry_safe(test, tmp, &head, list) {
		printk("%d\n", test->value);
		if (i++ > 100) {
			break;
		}
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
