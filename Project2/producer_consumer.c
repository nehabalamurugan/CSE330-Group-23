// CSE 330 Project 2
// Group 23

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/types.h>


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

// Counters
size_t task_count = 0;


// Task structs
typedef struct task_struct TaskStruct;
TaskStruct *producers;
TaskStruct *consumers;


// Node struct
typedef struct node {
    int pid;
    int uid;
    int item_num;
    long time;
    long start_time;
} Node;


// Buffer linked list
typedef struct buffer {
    Node node;
    struct buffer *next;
} Buffer;

// Buffer stack
Buffer *buffer;


static int producer(void *arg) {
    Node node;
    TaskStruct *task;

    // Iterate through the task list
    for_each_process(task) {

        // Check if the task uid matches
        if (task->cred->uid.val == uuid) {
            // Semaphore down
            down_interruptible(&empty);
            down_interruptible(&mutex);

            task_count ++;

            // Fill node
            node = (Node) {
                .pid = task->pid,
                .uid = task->cred->uid.val,
                .item_num = task_count,
                .time = 0,
                .start_time = task->start_time
            };

            // Add node to buffer
            if (buffer == NULL) {
                buffer->node = node;
                buffer->next = NULL;
            } else {
                Buffer *next = buffer;
                buffer->node = node;
                buffer->next = next;
            }

            // Print process info
            printk(KERN_INFO "Producer-1 Produced Item#-%d at buffer index: %zu for PID:%d", node.item_num, task_count, node.pid);

            // Semaphore up
            up(&mutex);
            up(&full);
        }
    }

    return 0;
}


int init_module(void) {
    // Initialize semaphores
    sema_init(&empty, buffSize);
    sema_init(&full, 0);
    sema_init(&mutex, 1);

    return 0;
}


void cleanup_module(void) {}
