// CSE 330 Project 2
// Group 23

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>


MODULE_LICENSE("GPL");


// Default params
static int buffSize = 1024;
static int prod = 1;
static int cons = 1;
static uid_t uuid = -1;

// Define params
module_param(buffSize, int, 0644);  // The buffer size
module_param(prod, int, 0644);      // number of producers(0 or 1)
module_param(cons, int, 0644);      // number of consumers(a non-negative number)
module_param(uuid, int, 0644);      // The UID of the user


int init_module(void) {
    return 0;
}


void cleanup_module(void) {}
