// CSE 330 Project 2
// Group 23

#include <linux/module.h>
#include <linux/kernel.h>


MODULE_LICENSE("GPL");


int init_module(void) {
    return 0;
}


void cleanup_module(void) {}
