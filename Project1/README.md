# Project 1

## Step 4

`make` before calling the kernal module.

```bash
sudo insmod main.ko; sudo dmesg | tail -n 1
sudo rmmod main.ko; sudo dmesg | tail -n 1
```

## Step 5

1. `cd linux-5.16`
2. `mkdir my_syscall`
    - Place syscall files here 
3. Update `linux-5.16/Makefile`
    - Update `core-y			+= kernel/ certs/ mm/ fs/ ipc/ security/ crypto/ my_syscall/`
4. Update `arch/x86/entry/syscalls/syscall_64.tbl`
    - Add `450 64      my_syscall      my_syscall` after the last row before 512
5. Update `include/linux/syscalls.h`
    - Add `asmlinkage long my_syscall(void);` to the end of the file right before the `#endif`
6. Update `include/uapi/asm-generic/unistd.h`
    - Add `#define __NR_my_syscall 450`
    - Add `__SYSCALL(__NR_my_syscall, my_syscall)`
7. Update `kernel/sys_ni.c`
    - Add `COND_SYSCALL(my_syscall);`
8. Recompile the kernel
    - `sudo make -j4 modules_install install`
9. Compile program `gcc syscall_test.c -o syscall_test`
10. Run `./syscall_test; sudo dmesg | tail -n 5`
