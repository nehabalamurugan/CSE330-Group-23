// CSE 330 Project 1
// Group 23
//
// main file

#include <linux/module.h>
#include <linux/kernel.h>


/**
 * Module function to run at load
*/
int main_init(void) {
    printk("Hello World!\n");
    return 0;
}

/**
 * Module function to run at unload
*/
void main_exit(void) {
    printk("Goodbye World!\n");
}


// Bind the functions
module_init(main_init);
module_exit(main_exit);
MODULE_LICENSE("GPL");
