#include <linux/kernel.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


int main() {
    syscall(450);
    return 0;
}
