#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm-generic/current.h>

struct lockdep_demo {
	int value;
	spinlock_t slock;
};

static int __init demo_init(void)
{
	struct lockdep_demo *lock = kmalloc(sizeof(struct lockdep_demo), GFP_KERNEL);
	if (!lock) {
		printk("kmalloc error\n");
		return PTR_ERR(lock);
	}

	spin_lock_init(&lock->slock);
	spin_lock(&lock->slock);

	/* 由于此时释放锁内存的时候，锁还没有释放，所以此处会报错 */
	kfree(lock);

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
