#include <sbi.h>
#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <sbi_rust.h>
#include <os/lock.h>
#include <os/sync.h>
#include <os/irq.h>
spinlock_t kernel_lock;


void wakeup_other_hart()
{
    uint64_t id = 2;
    sbi_send_ipi(&id); 
    __asm__ __volatile__ (
        "csrw sip, zero"
    );
}

// get current 
uint64_t get_current_cpu_id()
{
    return get_current_running()->save_context->core_id;
}

// lock kernel
void lock_kernel()
{
    while(atomic_cmpxchg(UNLOCKED, LOCKED, (ptr_t)&kernel_lock.status))
    {
        // printk("try enter kernel, the error place: %lx\n", get_current_running()->save_context->sepc); 
        // uint64_t stval;
        // __asm__ __volatile__("csrr %0, stval" : : "rK"(stval) : "memory");
        // uint64_t cause;
        // __asm__ __volatile__("csrr %0, scause" : : "rK"(cause) : "memory");
        // handle_other(get_current_running()->save_context, stval, cause);
    }
        
}

// unlock kernel
void unlock_kernel()
{
    kernel_lock.status = UNLOCKED;
}

void init_kernel_lock()
{
    kernel_lock.status = UNLOCKED;
}