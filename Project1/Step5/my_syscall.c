#include <linux/kernel.h>

asmlinkage long my_syscall(void) {
	printk("This is the new system call SangelkarBalamuruganAraniNair implemented.");
	return 0;
}
