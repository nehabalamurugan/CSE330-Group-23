/**
 * @file memory_manager.c
 * @author Group 23
 * @brief CSE 330 Project 3
 */

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/module.h>

#define AUTHOR "Neha Balamurgan, Punit Arani, Arnav Sangelkar, Siddhesh Nair"
#define DESCRIPTION "Memory Manager"

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DESCRIPTION);

// Kernal Arguments
int pid = 0;
module_param(pid, int, 0);

// Page Counters
unsigned int RSS;
unsigned int SWAP;
unsigned int WSS;

// Task Variables
struct task_struct *task;

// Timer Variables
unsigned long timer_interval_ns = 10e9; // 10-second timer
static struct hrtimer hr_timer;

/**
 * Finds the page table entry (pte) for a given virtual address in the provided
 * memory management structure (mm_struct).
 *
 * @param mm: Pointer to the mm_struct where the address translation is performed.
 * @param virt_addr: The virtual address for which the corresponding pte is needed.
 *
 * @return pte_t*: Pointer to the pte_t structure for the given virtual address.
 *                 Returns NULL if any intermediate table entry is bad or doesn't exist,
 *                 or if the mm_struct pointer is NULL.
 */
pte_t *find_page_table_entry(const struct mm_struct *const mm, const unsigned long virt_addr)
{
    // Handle NULL pointer mm_struct input
    if (!mm)
    {
        return NULL;
    }

    pgd_t *page_global_dir;
    p4d_t *page_4_level_dir;
    pmd_t *page_middle_dir;
    pud_t *page_upper_dir;
    pte_t *page_table_entry = NULL;

    // Get the page global directory entry
    page_global_dir = pgd_offset(mm, virt_addr);
    if (pgd_none(*page_global_dir) || pgd_bad(*page_global_dir))
    {
        return NULL;
    }

    // Get the 4-level page table directory entry
    page_4_level_dir = p4d_offset(page_global_dir, virt_addr);
    if (p4d_none(*page_4_level_dir) || p4d_bad(*page_4_level_dir))
    {
        return NULL;
    }

    // Get the page upper directory entry
    page_upper_dir = pud_offset(page_4_level_dir, virt_addr);
    if (pud_none(*page_upper_dir) || pud_bad(*page_upper_dir))
    {
        return NULL;
    }

    // Get the page middle directory entry
    page_middle_dir = pmd_offset(page_upper_dir, virt_addr);
    if (pmd_none(*page_middle_dir) || pmd_bad(*page_middle_dir))
    {
        return NULL;
    }

    // Get the page table entry for the given virtual address
    page_table_entry = pte_offset_map(page_middle_dir, virt_addr);
    if (!page_table_entry)
    {
        return NULL;
    }

    return page_table_entry;
}

/**
 * Tests and clears the "young" (accessed) bit in the given page table entry (pte).
 * Returns whether the bit was set before being cleared.
 *
 * @param vma: Pointer to the vm_area_struct containing the memory mapping information.
 * @param addr: The virtual address of the page corresponding to the pte.
 * @param pte_ptr: Pointer to the pte_t structure to test and clear the young bit.
 *
 * @return int: 1 if the young bit was set before being cleared, 0 otherwise.
 */
int ptep_test_and_clear_young(struct vm_area_struct *vmas, unsigned long addr, pte_t *ptep)
{
    int ret = 0;

    if (pte_young(*ptep))
    {
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *)&ptep->pte);
    }

    return ret;
}

/**
 * Performs a page table walk for a given task and updates the working set size (WSS).
 * This function iterates through all the virtual memory areas (VMAs) of a task,
 * finding the page table entries (PTEs) for each address, and checks if the pages
 * have been accessed recently.
 *
 * @param task: Pointer to the task_struct for which the page table walk is performed.
 */
void perform_page_table_walk(struct task_struct *task)
{
    // Handle NULL pointer task_struct input
    if (!task)
    {
        return;
    }

    // Get the mm_struct for the given task
    const struct mm_struct *mm = task->mm;

    // Get the first VMA for the given task
    const struct vm_area_struct *vma = mm->mmap;

    // Initialize the WSS to 0
    unsigned long WSS = 0;

    // Iterate through all the VMAs of the given task
    while (vma)
    {
        unsigned long address;
        for (address = vma->vm_start; address < vma->vm_end; address += PAGE_SIZE)
        {
            pte_t *pte = find_page_table_entry(mm, address);
            int was_accessed = ptep_test_and_clear_young(vma, address, pte);

            // Increase the working set size by 4 bytes (size of a pte_t)
            if (was_accessed)
            {
                WSS += 4;
            }
        }
        vma = vma->vm_next;
    }
}

/**
 * Restart callback function for an hrtimer. This function is called when the timer
 * expires, and it reschedules the timer based on a predefined interval.
 *
 * @param timer_for_restart: Pointer to the hrtimer that expired and needs to be restarted.
 *
 * @return enum hrtimer_restart: Always returns HRTIMER_RESTART, indicating that the timer
 *                                should be restarted with the updated expiration time.
 *
 * @note
 * In this implementation, the callback function performs the following tasks:
 *   1. Gets the current time.
 *   2. Sets the interval for the timer's next expiration.
 *   3. Updates the timer's expiration time to the new interval.
 *   4. Finds the task_struct for the given PID.
 *   5. Performs a page table walk for the task and updates the working set size (WSS).
 *   6. Prints the task's PID, resident set size (RSS), swap usage, and WSS.
 *   7. Resets the RSS, WSS, and swap usage values.
 *
 */
enum hrtimer_restart restart_callback(struct hrtimer *timer_for_restart)
{
    ktime_t currtime, interval;
    currtime = ktime_get();
    interval = ktime_set(0, timer_interval_ns);
    hrtimer_forward(timer_for_restart, currtime, interval);

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    pageWalkthrough(task);
    printk("PID-%d: RSS = %u KB SWAP = %u KB WSS = %u KB", pid, RSS, SWAP, WSS);
    RSS = 0;
    WSS = 0;
    SWAP = 0;

    return HRTIMER_RESTART;
}

// Initialize
static int __init initialize(void)
{
    ktime_t ktime;
    ktime = ktime_set(0, timer_interval_ns);
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = &restart_callback;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
    return 0;
}

// Exit module
static void __exit clean_exit(void)
{
    int ret;
    ret = hrtimer_cancel(&hr_timer);
    if (ret)
        printk("The timer was still in use...\n");
    printk("HR Timer module uninstalling\n");
}

module_init(initialize);
module_exit(clean_exit);
