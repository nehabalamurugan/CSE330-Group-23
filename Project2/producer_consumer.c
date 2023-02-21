// CSE 330 Project 2
// Group 23

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/semaphore.h>


#define AUTHORS     "Punit Sai Arani, Neha Balamurugan, Siddhesh Nair, Arnav Sangelkar"
#define DESCRIPTION "Project 2"


MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHORS);
MODULE_DESCRIPTION(DESCRIPTION);


// Default params
static int buffSize = 0;
static int prod = 0;
static int cons = 0;
static uid_t uuid = 0;

// Define params
module_param(buffSize, int, 0644);  // The buffer size
module_param(prod, int, 0644);      // number of producers(0 or 1)
module_param(cons, int, 0644);      // number of consumers(a non-negative number)
module_param(uuid, int, 0644);      // The UID of the user


// Semaphores
struct semaphore empty;
struct semaphore full;
struct semaphore mutex;


int init_module(void) {
    // Initialize semaphores
    sema_init(&empty, buffSize);
    sema_init(&full, 0);
    sema_init(&mutex, 1);

    return 0;
}


void cleanup_module(void) {}
