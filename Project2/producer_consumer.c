// CSE 330 Project 2
// Group 23

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/types.h>

#define AUTHORS                                                                \
  "Punit Sai Arani, Neha Balamurugan, Siddhesh Nair, Arnav Sangelkar"
#define DESCRIPTION "Project 2"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHORS);
MODULE_DESCRIPTION(DESCRIPTION);

// Default params
int buffSize = 0;
int prod = 0;
int cons = 0;
int uuid = 0;

// Define params
module_param(buffSize, int, 0644); // The buffer size
module_param(prod, int, 0644);     // number of producers(0 or 1)
module_param(cons, int, 0644);     // number of consumers(a non-negative number)
module_param(uuid, int, 0644);     // The UID of the user

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

// Error variable
int error;

// Struct Node
typedef struct node {
  int pid;
  int uuid;
  int time;
  int timeStart;
  int itemNum;
} Node;

// Buffer
Node buffer[1024];
size_t buffer_head = 0;
size_t buffer_tail = 0;

// Global Time
unsigned long long global_time = 0;

static int producer(void *arg) {
  Node produced;
  TaskStruct *task;

  for_each_process(task) {
    if (task->cred->uid.val == uuid) {
      task_count++;

      // Update the empty and mutex semaphores
      // This locks the buffer
      down_interruptible(&empty);
      down_interruptible(&mutex);

      // Build the the produced node
      produced.pid = task->pid;
      produced.itemNum = task_count;
      produced.uuid = producers->cred->uid.val;
      produced.time = 0;
      produced.timeStart = producers->start_time;

      // TODO: Update Total Time so that we can present it when the module is
      // unloaded

      // Add the produced node to the buffer
      buffer[buffer_head] = produced;

      // Print the produced item information to the kernel log
      printk(KERN_INFO
             "[Producer-1] Produced Item#-%zu at buffer index: %zu for PID: %d",
             task_count, buffer_head, task->pid);

      buffer_head = (buffer_head + 1) % buffSize;

      // Update the mutex and full semaphores
      // This unlocks the buffer
      up(&mutex);
      up(&full);
    }
  }
  return 0;
}

static int consumer(void *consumerData) {
  // Run the consumer task forever
  // Monitors the buffer and consumes items as they are produced
  while (1) {
    while (buffSize > 0) {
      // Update the full and mutex semaphores
      // This locks the buffer
      down_interruptible(&full);
      down_interruptible(&mutex);

      Node consumed = buffer[buffer_tail];

      // Find when when this particular task was started and then assign it
      for_each_process(consumers) {
        if (consumers->pid == consumed.pid) {
          consumed.time = ktime_get_ns() - consumers->start_time;
        }
      }

      int time = consumed.time;
      int hour = (time / (1000000000ULL * 3600)) % 24;
      int minute = (time / (1000000000ULL * 60)) % 60;
      int second = (time / 1000000000ULL) % 60;

      printk(KERN_INFO "[Consumer] Consumed Item#-%d on buffer index: %zu "
                       "PID:%d Elapsed Time- %d:%d:%d",
             consumed.itemNum, buffer_tail, consumed.pid, hour, minute, second);

      buffer_tail = (buffer_tail + 1) % buffSize;

      // Update the mutex and empty semaphores
      // This unlocks the buffer
      up(&mutex);
      up(&empty);
    }
  }
  return 0;
}

static int __init init_func(void) {
  // Set the global time
  global_time = ktime_get_ns();

  // Semaphore Initizaliations
  sema_init(&empty, buffSize);
  sema_init(&full, 0);
  sema_init(&mutex, 1);

  if (prod) {
    producers = kthread_run(producer, NULL, "Producer Thread");

    if (IS_ERR(producer)) {
      printk(KERN_INFO "Error creating producer thread. \n");
      return error;
    }
  }

  if (cons) {
    consumers = kthread_run(consumer, NULL, "Consumer Thread");

    if (IS_ERR(consumer)) {
      printk(KERN_INFO "Error creating consumer thread. \n");
    }
  }

  return 0;
}

static void __exit exit_func(void) {
  unsigned long long time = ktime_get_ns() - global_time;
  int hour = (time / (1000000000ULL * 3600)) % 24;
  int minute = (time / (1000000000ULL * 60)) % 60;
  int second = (time / 1000000000ULL) % 60;

  printk(KERN_INFO
         "The total elapsed time for all processes for UID %d is %d:%d:%d\n",
         uuid, hour, minute, second);
}

module_init(init_func);
module_exit(exit_func);
