/*
 * 验证当内核中 proc_test_show() proc_test_write() 返回-EINVAL 的时候
 * 用户态会返回什么返回值。
 * 
 * 验证表明不管内核态返回什么，用户态接收到的返回值都是(-1)
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FILE_NAME	"/proc/proc_demo/file1"

int fd;

void read_test(void)
{
	int  ret;
	char buff[256];

	memset(buff, 0, sizeof(buff));
	ret = read(fd, buff, sizeof(buff));
	if (ret > 0) {
		printf("ret %d buff %s\n", ret, buff);
	} else {
		printf("ret %d\n", ret);
	}
}

void write_test(void)
{
	int  ret;
	char buff[12];

	ret = write(fd, buff, sizeof(buff));

	printf("ret %d\n", ret);
}

int main()
{
	fd = open(FILE_NAME, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open error: %s\n", strerror(errno));
		return -1;
	}

	printf("read test...\n");
	read_test();

	printf("write test...\n");
	write_test();

	return 0;
}
