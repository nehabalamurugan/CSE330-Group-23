// CSE 330 Project 2
// Group 23

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/types.h>

#define AUTHORS "Punit Sai Arani, Neha Balamurugan, Siddhesh Nair, Arnav Sangelkar"
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
int count1 = 0;

// Task structs
typedef struct task_struct TaskStruct;
TaskStruct *producers;
TaskStruct *consumers;

// Error variable
int error;

// Struct Node
struct node {
  int pid;
  int uuid;
  int time;
  int timeStart;
  int itemNum;
};

// Buffer
struct node buffer[1000];

// Buffer position trackers
int in = 0;
int out = 0;

// Global Time
u64 tTime = 0;

static int producer(void *arg) {
  struct node temp;

  TaskStruct *task;
  // Debug Tools
  // printk(KERN_INFO "PRODUCER STARTED");
  // printk(KERN_INFO "%d", buffSize);

  for_each_process(task) {
    if (task->cred->uid.val == uuid) {

      task_count++;

      // Semaphore Updates
      down_interruptible(&empty);
      down_interruptible(&mutex);

      // assign values of the current producer task to temp
      temp.pid = task->pid;
      temp.itemNum = task_count;
      temp.uuid = producers->cred->uid.val;
      temp.time = 0;
      temp.timeStart = producers->start_time;

      // Update Total Time so that we can present it when the module is unloaded

      buffer[in] = temp;

      printk(KERN_INFO
             "[Producer-1] Produced Item#-%d at buffer index: %d for PID: %d",
             task_count, in, task->pid);
      in = in + 1;

      // Semaphore Updates
      up(&mutex);
      up(&full);
    }
  }
  // Debug Tools
  // printk(KERN_INFO "PRODUCER DONE");

  return 0;
}

static int consumer(void *consumerData) {

  // Consumer will run in a infinite loop unlike the producer
  while (1) {

    while (buffSize > 0) {

      count1++;

      // Semaphore Updates
      down_interruptible(&full);
      down_interruptible(&mutex);

      struct node consume = buffer[out];

      // Find when when this particular task was started and then assign it
      for_each_process(consumers) {
        if (consumers->pid == consume.pid) {
          consume.time = ktime_get_ns() - consumers->start_time;
        }
      }

      int elapsed = consume.time;
      int hour = elapsed / 3600000000000;
      elapsed = elapsed % 3600000000000;
      int minute = elapsed / 60000000000;
      elapsed = elapsed % 60000000000;
      int second = elapsed / 1000000000;

      out = (out++) % buffSize;
      printk(KERN_INFO "[Consumer] Consumed Item#-%d on buffer index: %d "
                       "PID:%d Elapsed Time- %d:%d:%d",
             consume.itemNum, out, consume.pid, hour, minute, second);

      // Semaphore Updates
      up(&mutex);
      up(&empty);
    }
  }
  return 0;
}

static int __init init_func(void) {
  // Semaphore Initizaliations
  sema_init(&empty, buffSize);
  sema_init(&full, 0);
  sema_init(&mutex, 1);

  if (prod) {
    // Start the K-Thread for the producer task
    producers = kthread_run(producer, NULL, "Producer Thread");

    if (IS_ERR(producer)) {
      printk(KERN_INFO "Error: cannot create thread producers. \n");
      error = PTR_ERR(producer);
      producers = NULL;
      return error;
    }
  }

  if (cons) {
    // Start the K-Thread for the consumer task
    consumers = kthread_run(consumer, NULL, "Consumer Thread");

    if (IS_ERR(consumer)) {
      printk(KERN_INFO "Error: cannot create thread consumers. \n");
      error = PTR_ERR(consumer);
      consumers = NULL;
      return error;
    }
  }

  return 0;
}

static void __exit exit_func(void) {
  // Converted elapsed into human readable time
  int elapsed = tTime;
  int hour = elapsed / 3600000000000;
  elapsed = elapsed % 3600000000000;
  int minute = elapsed / 60000000000;
  elapsed = elapsed % 60000000000;
  int second = elapsed / 1000000000;

  printk(KERN_INFO
         "The total elapsed time for all processes for UID %d is %d:%d:%d\n",
         uuid, hour, minute, second);
}

module_init(init_func);
module_exit(exit_func);
