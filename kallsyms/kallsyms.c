/*
 * 通过kallsyms_lookup() 查询符号表
 *
 * 编译通过内核列表:
 * 2.6.35.6
 *
 * 需要导出符号表 kallsyms_lookup.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

static int __look_for_me;
static int __look_for_us[8];

static int __init demo_init(void)
{
	const char *name;
	char str[KSYM_NAME_LEN];

	memset(str, 0, sizeof(str));
	name = kallsyms_lookup((unsigned long)&__look_for_me, NULL, NULL, NULL, str);
	printk("name is %s\n", name);
	name = kallsyms_lookup((unsigned long)&__look_for_us[2], NULL, NULL, NULL, str);
	printk("name is %s\n", name);


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
