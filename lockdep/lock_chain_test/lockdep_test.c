#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm-generic/current.h>

spinlock_t lock1;
spinlock_t lock2;

void debug_info(void)
{
	int i;

	for (i = 0; i < current->lockdep_depth; i++) {
		printk("current %s i %d held_locks[i].prev_chain_key %llu\n", current->comm, i, current->held_locks[i].prev_chain_key);
	}
	printk("current %s %llu\n", current->comm, current->curr_chain_key);
}

void debug_func(void)
{
	spin_lock(&lock1); debug_info();
	spin_lock(&lock2); debug_info();
	spin_unlock(&lock2);
	spin_unlock(&lock1);
}

static int __init demo_init(void)
{
	printk("init...\n");
	spin_lock_init(&lock1);
	spin_lock_init(&lock2);

	debug_func();

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
