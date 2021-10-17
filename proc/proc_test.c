/*
 * 1. proc 函数接口，是如何执行的
 * 2. 文件系统中几个重要的数据结构
 *
 * 编译通过内核列表:
 * 5.0.0
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_DEMO_NAME	"proc_demo"
#define PROC_DEMO_FILE1	"file1"
#define PROC_DEMO_FILE2	"file2"

#define DEBUG

static int value1 = 1;
static int value2 = 2;
static struct proc_dir_entry *proc_demo;

static int proc_test_show(struct seq_file *s, void *offset)
{
	void *v = s->private;

#ifdef DEBUG
	/* 不论在这里返回的是什么返回值，最终到用户态的read() 都只会返回(-1) */
	return -EINVAL;
#endif

	if (v == &value1) {
		seq_printf(s, "%d\n", value1);
	} else if (v == &value2) {
		seq_printf(s, "%d\n", value2);
	} else {
		seq_printf(s, "nihao\n");
	}

	return 0;
}

static int proc_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_test_show, PDE_DATA(inode));
}

static ssize_t proc_test_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *pos)
{
	int  ret;
	int  val;
	char kbuff[32];
	int *pvalue = (int *)((struct seq_file *)file->private_data)->private;

	memset(kbuff, 0, sizeof(kbuff));

	if (count > sizeof(kbuff))
		return -EINVAL;

	/* 不论在这里返回什么返回值，到用户态都write() 都只是返回(-1) */
	if (copy_from_user(kbuff, buffer, sizeof(kbuff) > count ? count : sizeof(kbuff)))
		return -EFAULT;

	ret = kstrtoint(kbuff, 0, &val);
	if (ret != 0) {
		return ret;
	}
	if (val >= 100 && val <= 10000) {
		*pvalue = val;
	} else {
		*pvalue = INT_MAX;
	}

	return count;
}

/* 此处这些函数的作用？ */
static struct file_operations proc_test_ops = {
	.owner	=	THIS_MODULE,
	.open	=	proc_test_open,
	.write	=	proc_test_write,
	.read	=	seq_read,
	.llseek	=	seq_lseek,
	.release=	single_release,
};

static int __init proc_test_init(void)
{
	struct proc_dir_entry *proc_demo_file1;
	struct proc_dir_entry *proc_demo_file2;

	proc_demo = proc_mkdir(PROC_DEMO_NAME, NULL);
	if (proc_demo == NULL) {
		goto out;
	}

	/* 通过最后的指针指明要访问的数据是value1，还是value2 */
	/* TODO: 这里的实现？ */
	proc_demo_file1 = proc_create_data(PROC_DEMO_FILE1, 0666, proc_demo, &proc_test_ops, &value1);
	if (!proc_demo_file1) {
		goto out1;
	}

	proc_demo_file2 = proc_create_data(PROC_DEMO_FILE2, 0666, proc_demo, &proc_test_ops, &value2);
	if (!proc_demo_file2) {
		goto out2;
	}

	return 0;
out2:
	remove_proc_entry(PROC_DEMO_FILE1, proc_demo);
out1:
	remove_proc_entry(PROC_DEMO_NAME, NULL);
out:
	return -1;
}

static void __exit proc_test_exit(void)
{
	remove_proc_entry(PROC_DEMO_FILE2, proc_demo);
	remove_proc_entry(PROC_DEMO_FILE1, proc_demo);
	remove_proc_entry(PROC_DEMO_NAME, NULL);
}

module_init(proc_test_init);
module_exit(proc_test_exit);

MODULE_LICENSE("GPL");  
MODULE_AUTHOR("yuchao");  
