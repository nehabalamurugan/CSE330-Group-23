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

pte_t *pteFinder(const struct mm_struct *const mm, const unsigned long address)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pud_t *pud;
    pte_t *ptep;

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd) || pgd_bad(*pgd))
    { // check if pgd is bad or does not exist
        return ptep;
    }

    p4d = p4d_offset(pgd, address); // get p4d from from pgd and the page address
    if (p4d_none(*p4d) || p4d_bad(*p4d))
    { // check if p4d is bad or does not exist
        return ptep;
    }

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud) || pud_bad(*pud))
    { // check if pud is bad or does not exist
        return ptep;
    }

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd) || pmd_bad(*pmd))
    { // check if pmd is bad or does not exist
        return ptep;
    }

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    if (!ptep)
    { // check if pte does not exist
        return ptep;
    }

    if (pte_present(*ptep))
    {
        RSS = RSS + 4;
    }
    else
    {
        SWAP = SWAP + 4;
    }

    return ptep;
}

int ptep_test_and_clear_young(struct vm_area_struct *vmas, unsigned long addr, pte_t *ptep)
{
    int ret = 0;
    if (pte_young(*ptep))
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *)&ptep->pte);
    return ret;
}

void pageWalkthrough(struct task_struct *taskParam)
{
    if (taskParam)
    {
        const struct mm_struct *mma = taskParam->mm;
        const struct vm_area_struct *vmas = mma->mmap;
        while (vmas)
        {
            unsigned long address;
            for (address = vmas->vm_start; address < vmas->vm_end; address += PAGE_SIZE)
            {
                pte_t *pte = pteFinder(mma, address);
                int wasAccessed = ptep_test_and_clear_young(vmas, address, pte);
                if (wasAccessed)
                {
                    WSS = WSS + 4;
                }
            }
            vmas = vmas->vm_next;
        }
    }
}

enum hrtimer_restart restart_callback(struct hrtimer *timer_for_restart)
{
    ktime_t currtime, interval;
    currtime = ktime_get();
    interval = ktime_set(0, timer_interval_ns);
    hrtimer_forward(timer_for_restart, currtime, interval);

    // WALKING PAGES
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    pageWalkthrough(task);
    printk("PID-%d: RSS = %u KB SWAP = %u KB WSS = %u KB", pid, RSS, SWAP, WSS);
    RSS = 0;
    WSS = 0;
    SWAP = 0;
    // -----------

    return HRTIMER_RESTART;
}

static int __init initialize(void)
{
    ktime_t ktime;
    ktime = ktime_set(0, timer_interval_ns);
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = &restart_callback;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
    return 0;
}

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
