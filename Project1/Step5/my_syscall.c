#include <linux/kernel.h>
#include <linux/syscalls.h>


SYSCALL_DEFINE0(my_syscall) {
	printk("This is the new system call AraniBalamuruganNairSangelkar implemented.");
	return 0;
}
