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
module_param(buffSize, int, 0); // The buffer size
module_param(prod, int, 0);     // number of producers(0 or 1)
module_param(cons, int, 0);     // number of consumers(a non-negative number)
module_param(uuid, int, 0);     // The UID of the user

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
  int timeStart;
  int itemNum;
} Node;

// Buffer
Node buffer[1024];
size_t buffer_head = 0;
size_t buffer_tail = 0;

//consumers array
TaskStruct *consumersArr[1024];

// Global Time
unsigned long long global_time = 0;

static int producer(void *arg) {
  Node produced;
  TaskStruct *task;
    
    //Check if the buffer is locked
    if(down_interruptible(&empty) && down_interruptible(&mutex)){
      return 0;
    }

    for_each_process(task) {
        if (task->cred->uid.val == uuid) {
          task_count++;

          // Build the the produced node
          produced.pid = task->pid;
          produced.itemNum = task_count;
          produced.uuid = task->cred->uid.val;
          produced.timeStart = task->start_time;

          // TODO: Update Total Time so that we can present it when the module is
          // unloaded

          // Add the produced node to the buffer
          buffer[buffer_head] = produced;

          // Print the produced item information to the kernel log
          printk(KERN_INFO
                "[Producer-1] Produced Item#-%zu at buffer index: %zu for PID: %d",
                task_count, buffer_head, task->pid);

          buffer_head++;
        }
      }
        // Update the mutex and full semaphores
        // This unlocks the buffer
        up(&mutex);
        up(&full);

        return 1;

}

static int consumer(void *consumerData) {
  // Run the consumer task forever
  // Monitors the buffer and consumes items as they are produced

  if(down_interruptible(&empty) && down_interruptible(&mutex)){
      return 0;
  }
  
  while(!kthread_should_stop()){

    while (buffer_head >= 0) {
      Node consumed = buffer[buffer_head];

      unsigned long long time = ktime_get_ns() - consumed.timeStart;

      global_time = global_time + time; //adding the time to the global time

      int hour = time / 3600000000000;
      time = time % 3600000000000;
      int minute = time / 60000000000;
      time = time % 60000000000;
      int second = time / 1000000000;

      printk(KERN_INFO "[Consumer] Consumed Item#-%d on buffer index: %zu "
                       "PID:%d Elapsed Time- %d:%d:%d",
             consumed.itemNum, buffer_head, consumed.pid, hour, minute, second);

      buffer_head--;
    }
  }
      // Update the mutex and empty semaphores
      // This unlocks the buffer
      up(&mutex);
      up(&empty);

  return 1;
}

static int __init init_func(void) {
  // Set the global time
  global_time = 0;

  // Semaphore Initizaliations
  sema_init(&empty, buffSize);
  sema_init(&full, 0);
  sema_init(&mutex, 1);

  if(prod){
    producers = kthread_run(producer, NULL, "Producer Thread");
    if (IS_ERR(producer)) {
      printk(KERN_INFO "Error creating producer thread. \n");
      return error;
    }
  }

  for(int i = 0; i < cons; i++){
    consumersArr[i] = kthread_run(consumer, NULL, "Consumer Thread");

    if (IS_ERR(consumer)) {
      printk(KERN_INFO "Error creating consumer thread. \n");
    }
  }

  return 0;
}

static void __exit exit_func(void) {
  unsigned long long time = global_time;

  int hour = time / 3600000000000;
  time = time % 3600000000000;
  int minute = time / 60000000000;
  time = time % 60000000000;
  int second = time / 1000000000;

  printk(KERN_INFO
         "The total elapsed time for all processes for UID %d is %d:%d:%d\n",
         uuid, hour, minute, second);

  //release locks
  up(&mutex);
  up(&empty);
  up(&full);

  // Stop the threads
  kthread_stop(producers);
  for(int i = 0; i < cons; i++){
    kthread_stop(consumersArr[i]);
  }

}

module_init(init_func);
module_exit(exit_func);
