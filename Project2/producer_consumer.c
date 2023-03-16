// CSE 330 Project 2
// Group 23

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
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

// Task Struct
typedef struct task_struct TaskStruct;

// Error variable
int error;

// Task Struct Linked List
typedef struct task_struct_ll {
  struct task_struct *task;
  struct task_struct_ll *next;
} TaskStructLL;

// Buffer Linked List
typedef struct buffer_ll {
  struct task_struct_ll *task_list;
  size_t capacity;
} BufferLL;

// Initialize Buffer
BufferLL *buffer = NULL;

// Kernel Thread Structs
TaskStruct *prod_thread;
TaskStructLL *cons_threads = NULL;

// Global Variables
size_t total_time = 0;
size_t total_consumed = 0;

// producer function
static int producer(void *arg) {
  TaskStruct *task;
  size_t task_count = 0;

  for_each_process(task) {
    if (task->cred->uid.val == uuid) {
      task_count++;

      // Check if the buffer is locked
      if (down_interruptible(&empty) || down_interruptible(&mutex)) {
        break;
      }

      // Create or Add to the buffer
      if (buffer->task_list == NULL) {
        buffer->task_list = kmalloc(sizeof(TaskStructLL), GFP_KERNEL);
        buffer->task_list->task = task;
        buffer->task_list->next = NULL;
        buffer->capacity = 1;
      } else {
        // Traverse to the end of the buffer
        TaskStructLL *temp = buffer->task_list;
        while (temp->next != NULL) {
          temp = temp->next;
        }
        temp->next = kmalloc(sizeof(TaskStructLL), GFP_KERNEL);
        temp->next->task = task;
        temp->next->next = NULL;
        buffer->capacity++;
      }

      // Print the produced item information to the kernel log
      printk(KERN_INFO
             "[Producer-1] Produced Item#-%zu at buffer index: %zu for PID: %d",
             task_count, buffer->capacity, task->pid);

      // Update the mutex and full semaphores
      // This unlocks the buffer
      up(&mutex);
      up(&full);
    }
  }

  return 0;
}

// consumer function
static int consumer(void *consumerData) {
  TaskStructLL *consumed;
  size_t consumed_count = 0;

  // Run the consumer task forever
  // Monitors the buffer and consumes items as they are produced
  while (!kthread_should_stop()) {
    if (down_interruptible(&empty) && down_interruptible(&mutex)) {
      break;
    }

    if (buffer != NULL && buffer->task_list != NULL && buffer->capacity > 0) {
      consumed = buffer->task_list;

      // Remove the first item from the buffer
      buffer->task_list = buffer->task_list->next;
      buffer->capacity--;

      // Get the time of the consumed item
      u64 time = ktime_get_ns() - consumed->task->start_time;
      u64 seconds = time / 1000000000;
      int hour = seconds / 3600;
      int minute = (seconds % 3600) / 60;
      int second = (seconds % 3600) % 60; 

      // Update the total time and consumed count
      total_time += seconds;
      total_consumed++;

      // Print the consumed item information to the kernel log
      printk(KERN_INFO "[Consumer] Consumed Item#-%zu on buffer index: %zu "
                       "PID:%d Elapsed Time- %d:%d:%d",
             total_consumed, buffer->capacity, consumed->task->pid, hour,
             minute, second);

      // Free the consumed item
      consumed_count++;

      // Update the mutex and empty semaphores
      // This unlocks the buffer
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

  // Initialize the buffer
  buffer = kmalloc(sizeof(BufferLL), GFP_KERNEL);

  // Start the producer thread
  if (prod) {
    prod_thread = kthread_run(producer, NULL, "Producer-1");
    if (IS_ERR(prod_thread)) {
      printk(KERN_INFO "Error creating producer thread. \n");
      return PTR_ERR(prod_thread);
    }
  }

  // Start the consumer threads
  TaskStruct *cons_thread;
  for (int i = 0; i < cons; i++) {
    cons_thread = kthread_run(consumer, NULL, "Consumer");
    if (IS_ERR(cons_thread)) {
      printk(KERN_INFO "Error creating consumer thread. \n");
      return PTR_ERR(cons_thread);
    }

    // Add the consumer thread to the list
    if (cons_threads == NULL) {
      cons_threads = kmalloc(sizeof(TaskStructLL), GFP_KERNEL);
      cons_threads->task = cons_thread;
      cons_threads->next = NULL;
    } else {
      // Traverse to the end of the list
      TaskStructLL *temp = cons_threads;
      while (temp->next != NULL) {
        temp = temp->next;
      }
      temp->next = kmalloc(sizeof(TaskStructLL), GFP_KERNEL);
      temp->next->task = cons_thread;
      temp->next->next = NULL;
    }
  }

  return 0;
}

static void __exit exit_func(void) {
  TaskStructLL *temp_buffer;
  TaskStructLL *temp_cons;

  // Free the buffer
  temp_buffer = buffer->task_list;
  while (temp_buffer != NULL) {
    TaskStructLL *free_buffer = temp_buffer;
    temp_buffer = temp_buffer->next;
    kfree(free_buffer);
  }
  kfree(buffer);

  u64 seconds = total_time / 1000000000;
  int hour = seconds / 3600;
  int minute = (seconds % 3600) / 60;
  int second = (seconds % 3600) % 60;

  printk(KERN_INFO
         "The total elapsed time for all processes for UID %d is %d:%d:%d\n",
         uuid, hour, minute, second);
}

module_init(init_func);
module_exit(exit_func);
