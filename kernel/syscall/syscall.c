#include <os/syscall.h>
#include <os/irq.h>
#include <os/stdio.h>
#include <os/sched.h>
long (*syscall[NUM_SYSCALLS])();


void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    regs->sepc = regs->sepc + 4;
    if (syscall[regs->regs[17]] == handle_miss)
        handle_miss(regs, interrupt, cause);

    // debug_info(regs, interrupt, cause);
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],
                                              regs->regs[11],
                                              regs->regs[12],
                                              regs->regs[13],
                                              regs->regs[14],
                                              regs->regs[15]);
    // printk("syscall end\n");
}
/*
 static inline _u64 internal_syscall(long n, _u64 _a0, _u64 _a1, _u64 _a2, _u64 _a3, _u64 _a4, _u64 _a5) {    
    register _u64 a0 asm("a0") = _a0;    
    register _u64 a1 asm("a1") = _a1;    
    register _u64 a2 asm("a2") = _a2;    
    register _u64 a3 asm("a3") = _a3;    
    register _u64 a4 asm("a4") = _a4;    
    register _u64 a5 asm("a5") = _a5;    
    register long syscall_id asm("a7") = n;    
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));
    return a0;
    }
*/