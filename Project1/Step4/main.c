// CSE 330 Project 1
// Group 23

#include <linux/module.h>
#include <linux/kernel.h>


/**
 * Module function to run at load
*/
int main_init(void) {
    printk("[Group-23][Punit Sai Arani, Neha Balamurugan, Siddhesh Nair, Arnav Sangelkar] Hello, I am Punit, a student of CSE330 Spring 2023.\n");
    return 0;
}


/**
 * Module function to run at unload
*/
void main_exit(void) {
    printk("Goodbye Group 23!\n");
}


// Bind the functions
module_init(main_init);
module_exit(main_exit);
MODULE_LICENSE("GPL");
