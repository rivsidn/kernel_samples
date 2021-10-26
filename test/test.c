#include <linux/kernel.h>
#include <linux/module.h>

#include "ip_defend_flood,h"

static int __init demo_init(void)
{
	return 0;
}
      
static void __exit demo_exit(void)
{
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchao");
