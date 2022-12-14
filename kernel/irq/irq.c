#include <os/irq.h>
#include <os/time.h>
#include <fs/fat32.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <swap/swap.h>
#include <sys/special_ctx.h>
#include <assert.h>
#include <sbi.h>
#include <sbi_rust.h>
#include <screen.h>
#include <csr.h>
#include <sdcard.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;
//debug
char*syscall_name[512];

void reset_irq_timer()
{
    screen_reflush();
    check_sleeping();
    check_futex_timeout();
    itimer_check();
    sbi_set_timer(get_ticks() + time_base / 100);
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    // printk("interrupt entry, sepc: %lx\n", regs->sepc);

    update_utime();
    if ((cause & SCAUSE_IRQ_FLAG) == SCAUSE_IRQ_FLAG) 
    {
        irq_table[regs->scause & ~(SCAUSE_IRQ_FLAG)](regs, stval, cause);
    } else
    {
        // ignore S
        if ((regs->sstatus & SR_SPP) != 0 && ((pcb_t *)get_current_running())->pid)
            handle_other(regs, stval, cause);        
        exc_table[regs->scause](regs, stval, cause);
    }
    update_stime();
    // printk("interrupt end\n");
}

// handle int
void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

// handle page fault inst
void handle_page_fault_inst(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("inst page fault: %lx\n",stval);
    pcb_t * current_running = get_current_running();
    uint64_t fault_addr = stval;
    if(check_W_SD_and_set_AD(fault_addr, current_running->pgdir, LOAD) == ALLOC_NO_AD){
        local_flush_tlb_all();
        return;
    }  
    printk(">[Error] can't resolve the inst page fault with 0x%x!\n",stval);
    handle_other(regs,stval,cause);
}

// handle page fault load
void handle_page_fault_load(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("load page fault: %lx\n",stval);
    pcb_t * current_running = get_current_running();
    uint64_t fault_addr = stval;  
    uint64_t kva;
    switch (check_W_SD_and_set_AD(fault_addr, current_running->pgdir, LOAD))
    {
    case ALLOC_NO_AD:
        local_flush_tlb_all();
        break;
    case IN_SD:
        // TODO
        if (status_ctx)
        {
            deal_in_SD(fault_addr);
            local_flush_tlb_all();            
        } else assert(0);

        break; 
    case NO_ALLOC:
        if (!is_legal_addr(stval))
            handle_other(regs, stval, cause);
        // deal the no alloc
        deal_no_alloc(fault_addr, LOAD);
        current_running->pge_num++;
        local_flush_tlb_all();
        break;              
    default:
        break;
    };
    // printk("load page end\n");
}

// handle page fault store
void handle_page_fault_store(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("store page fault: %lx\n",stval);
    if (is_kva(stval))
        return;
    pcb_t * current_running = get_current_running();
    uint64_t fault_addr = stval; 
    uint64_t kva;  
    switch (check_W_SD_and_set_AD(fault_addr, current_running->pgdir, STORE))
    {
    case ALLOC_NO_AD:
        local_flush_tlb_all();
        break;
    case IN_SD:
        // TODO
        if (status_ctx)
        {
            deal_in_SD(fault_addr);
            local_flush_tlb_all();            
        } else assert(0);

        local_flush_tlb_all();
        break; 
    case IN_SD_NO_W:
        assert(0);
        break; 
    case NO_W:
        deal_no_W(fault_addr);
        local_flush_tlb_all();
        break;
    case NO_ALLOC:
        // deal no alloc 
        deal_no_alloc(fault_addr, STORE);
        current_running->pge_num++;  
        local_flush_tlb_all();
        break;      
    default:
        break;
    }
    // printk("store page end\n");
}


void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for ( int i = 0; i < IRQC_COUNT; i++)
    {
        irq_table[i] = &handle_other;
    }
    for ( int i = 0; i < EXCC_COUNT; i++)
    {
        exc_table[i] = &handle_other;
    }
    irq_table[IRQC_S_TIMER]          = &handle_int;
    // irq_table[IRQC_S_SOFT]           = &disk_intr;
    exc_table[EXCC_SYSCALL]          = &handle_syscall;
    exc_table[EXCC_LOAD_ACCESS]      = &handle_page_fault_load;    
    exc_table[EXCC_LOAD_PAGE_FAULT]  = &handle_page_fault_load;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_page_fault_store;
    exc_table[EXCC_STORE_ACCESS]     = &handle_page_fault_store;
    // exc_table[EXCC_INST_PAGE_FAULT]  = &handle_page_fault_inst;
    setup_exception();
    //debug
    init_syscall_name();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    pcb_t * pcb = get_current_running();//(pcb_t *)regs->regs[4];
    prints("name: %s, pid: %ld\n", pcb->name, pcb->pid);
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            prints("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        prints("\n\r");
    }
    prints("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    prints("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    prints("sepc: 0x%lx\n\r", regs->sepc);
    prints("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    prints("[Backtrace]\n\r");
    prints("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    // while (fp > 0x10000) {
    //     uintptr_t prev_ra = *(uintptr_t*)(fp-8);
    //     uintptr_t prev_fp = *(uintptr_t*)(fp-16);

    //     printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

    //     fp = prev_fp;
    // }
    screen_reflush();
    assert(0);
}

void handle_miss(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // the tp will be other thingss
    pcb_t * pcb = get_current_running();//(pcb_t *)regs->regs[4];
    prints("name: %s, pid: %ld\n", pcb->name, pcb->pid);    
    prints("handle undefined syscall_number: %ld\n", regs->regs[17]);
    prints("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    prints("[Backtrace]\n\r");
    prints("  addr: 0x%lx sp: 0x%lx fp: 0x%lx\n\r", regs->regs[1] - 4, regs->regs[2], regs->regs[8]);
    screen_reflush();
    assert(0);
}

void deal_s_trap(pcb_t * tp, uint64_t stval, uint64_t cause){
    if(tp->pid ){
        if(cause == EXCC_STORE_PAGE_FAULT || \
           cause == EXCC_LOAD_PAGE_FAULT || \
           cause ==  EXCC_STORE_ACCESS ){
            if(tp->save_context->sstatus & SR_SPP){
                #ifdef QEMU
                    __asm__ volatile (
                            "la t0, 0x00040000\n"
                            "csrw sstatus, t0\n"
                            );
                #else
                    tp->save_context->sstatus = 0;
                    __asm__ volatile ("csrwi sstatus, 0\n");

                #endif                
            }
        }
    }
}

void debug_info(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // printk("==============debug-info start==============\n\r");
    pcb_t * pcb = get_current_running();//(pcb_t *)regs->regs[4];
    if(regs->regs[17] == 165 || regs->regs[17] == 113){
        // don't print getrusage and clock gettime
        return;
    }
    printk("core %d pid %d entering syscall: %s\n",do_getpid() == current_running_master->pid? 0: 1, do_getpid(), syscall_name[regs->regs[17]]);
    // pcb_t * pcb = get_current_running();//(pcb_t *)regs->regs[4];
    // printk("process name: %s, pid:%d\n", pcb->name, pcb->pid);
    //printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",regs->sstatus, regs->sbadaddr, regs->scause);
    // printk("stval: 0x%lx cause: %lx\n\r",stval, cause);
    // printk("sepc: 0x%lx\n\r", regs->sepc);
    //printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    // uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    // printk("[Backtrace]\n\r");
    // printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    // while (fp > USER_STACK_ADDR - PAGE_SIZE) {
    //     uintptr_t prev_ra = *(uintptr_t*)(fp-8);
    //     uintptr_t prev_fp = *(uintptr_t*)(fp-16);
    //     if(prev_ra > 0x100000){
    //         break;
    //     }
    //     if(prev_fp < 0x10000){
    //         break;
    //     }
    //     printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);
    //     fp = prev_fp;
        
    // }
    // printk("==============debug-info end================\n\r");
}

char *syscall_array[] = {
    "SYS_getcwd      ",     
    "SYS_dup         ",      
    "SYS_dup3        ",     
    "SYS_mkdirat     ",      
    "SYS_unlinkat    ",       
    "SYS_linkat      ",       
    "SYS_umount2     ",       
    "SYS_mount       ",       
    "SYS_chdir       ",       
    "SYS_openat      ",      
    "SYS_close       ",       
    "SYS_pipe2       ",         
    "SYS_getdents64  ",       
    "SYS_read        ",      
    "SYS_write       ",       
    "SYS_fstat       ",     
    "SYS_exit        ",      
    "SYS_nanosleep   ",      
    "SYS_sched_yield ",      
    "SYS_times       ",      
    "SYS_uname       ",      
    "SYS_gettimeofday",      
    "SYS_getpid      ",     
    "SYS_getppid     ",      
    "SYS_brk         ",      
    "SYS_munmap      ",      
    "SYS_clone       ",       
    "SYS_execve      ",      
    "SYS_mmap        ",       
    "SYS_wait4       ",      
    "SYS_exec          ",      
    "SYS_screen_write  ",      
    "SYS_screen_reflush",      
    "SYS_screen_clear  ",      
    "SYS_get_timer     ",      
    "SYS_get_tick      ",      
    "SYS_shm_pageget   ",      
    "SYS_shm_pagedt    ",      
    "SYS_disk_read     ",      
    "SYS_disk_write    ",      
    "SYS_fcntl             ",  
    "SYS_ioctl             ",  
    "SYS_statfs            ",  
    "SYS_fstatfs           ",  
    "SYS_lseek             ",  
    "SYS_readv             ",  
    "SYS_writev            ",  
    "SYS_pread             ",  
    "SYS_pwrite            ",  
    "SYS_ppoll             ",  
    "SYS_fstatat           ",  
    "SYS_utimensat         ",  
    "SYS_exit_group        ",  
    "SYS_set_tid_address   ",  
    "SYS_futex             ",  
    "SYS_set_robust_list   ",  
    "SYS_get_robust_list   ",  
    "SYS_clock_gettime     ",  
    "SYS_sched_setscheduler",  
    "SYS_sched_getaffinity ",  
    "SYS_kill              ",  
    "SYS_tkill             ",  
    "SYS_rt_sigaction      ",  
    "SYS_rt_sigprocmask    ",  
    "SYS_rt_sigtimedwait   ",  
    "SYS_rt_sigreturn      ",  
    "SYS_setsid            ",  
    "SYS_getrlimit         ",  
    "SYS_setrlimit         ",  
    "SYS_geteuid           ",  
    "SYS_getegid           ",  
    "SYS_gettid            ",  
    "SYS_sysinfo           ",  
    "SYS_socket            ",  
    "SYS_bind              ",  
    "SYS_listen            ",  
    "SYS_accept            ",  
    "SYS_connect           ",  
    "SYS_getsockname       ",  
    "SYS_sendto            ",  
    "SYS_recvfrom          ",  
    "SYS_setsockopt        ",  
    "SYS_mremap            ",  
    "SYS_mprotect          ",  
    "SYS_madvise           ",  
    "SYS_prlimit           ",
    "SYS_preload           ",
    "SYS_membarrier        ",
    "SYS_faccessat         ",
    "SYS_sendfile          ",
    "SYS_pselect6          ",
    "SYS_readlinkat        ",
    "SYS_fsync             ",
    "SYS_setitimer         ",
    "SYS_syslog            ",
    "SYS_tgkill            ",
    "SYS_getrusage         ",
    "SYS_vm86              ",
    "SYS_getuid            ",
    "SYS_getgid            ",
    "SYS_msync             ",
    "SYS_renameat2         ",
    "SYS_extend            "
};

void init_syscall_name(){
    syscall_name[ 17] = syscall_array[0];     
    syscall_name[ 23] = syscall_array[1];      
    syscall_name[ 24] = syscall_array[2];     
    syscall_name[ 34] = syscall_array[3];      
    syscall_name[ 35] = syscall_array[4];       
    syscall_name[ 37] = syscall_array[5];       
    syscall_name[ 39] = syscall_array[6];       
    syscall_name[ 40] = syscall_array[7];       
    syscall_name[ 49] = syscall_array[8];       
    syscall_name[ 56] = syscall_array[9];      
    syscall_name[ 57] = syscall_array[10];       
    syscall_name[ 59] = syscall_array[11];         
    syscall_name[ 61] = syscall_array[12];       
    syscall_name[ 63] = syscall_array[13];      
    syscall_name[ 64] = syscall_array[14];       
    syscall_name[ 80] = syscall_array[15];     
    syscall_name[ 93] = syscall_array[16];      
    syscall_name[101] = syscall_array[17];      
    syscall_name[124] = syscall_array[18];      
    syscall_name[153] = syscall_array[19];      
    syscall_name[160] = syscall_array[20];      
    syscall_name[169] = syscall_array[21];      
    syscall_name[172] = syscall_array[22];     
    syscall_name[173] = syscall_array[23];      
    syscall_name[214] = syscall_array[24];      
    syscall_name[215] = syscall_array[25];      
    syscall_name[220] = syscall_array[26];       
    syscall_name[221] = syscall_array[27];      
    syscall_name[222] = syscall_array[28];       
    syscall_name[260] = syscall_array[29];      
    syscall_name[243] = syscall_array[30];      
    syscall_name[244] = syscall_array[31];      
    syscall_name[245] = syscall_array[32];      
    syscall_name[246] = syscall_array[33];      
    syscall_name[247] = syscall_array[34];      
    syscall_name[248] = syscall_array[35];      
    syscall_name[249] = syscall_array[36];      
    syscall_name[250] = syscall_array[37];      
    syscall_name[251] = syscall_array[38];      
    syscall_name[252] = syscall_array[39];      
    syscall_name[ 25] = syscall_array[40];  
    syscall_name[ 29] = syscall_array[41];  
    syscall_name[ 43] = syscall_array[42];  
    syscall_name[ 44] = syscall_array[43];  
    syscall_name[ 62] = syscall_array[44];  
    syscall_name[ 65] = syscall_array[45];  
    syscall_name[ 66] = syscall_array[46];  
    syscall_name[ 67] = syscall_array[47];  
    syscall_name[ 68] = syscall_array[48];  
    syscall_name[ 73] = syscall_array[49];  
    syscall_name[ 79] = syscall_array[50];  
    syscall_name[ 88] = syscall_array[51];  
    syscall_name[ 94] = syscall_array[52];  
    syscall_name[ 96] = syscall_array[53];  
    syscall_name[ 98] = syscall_array[54];  
    syscall_name[ 99] = syscall_array[55];  
    syscall_name[100] = syscall_array[56];  
    syscall_name[113] = syscall_array[57];  
    syscall_name[119] = syscall_array[58];  
    syscall_name[123] = syscall_array[59];  
    syscall_name[129] = syscall_array[60];  
    syscall_name[130] = syscall_array[61];  
    syscall_name[134] = syscall_array[62];  
    syscall_name[135] = syscall_array[63];  
    syscall_name[137] = syscall_array[64];  
    syscall_name[139] = syscall_array[65];  
    syscall_name[157] = syscall_array[66];  
    syscall_name[163] = syscall_array[67];  
    syscall_name[164] = syscall_array[68]; 
    syscall_name[175] = syscall_array[69];  
    syscall_name[177] = syscall_array[70];  
    syscall_name[178] = syscall_array[71];  
    syscall_name[179] = syscall_array[72];  
    syscall_name[198] = syscall_array[73];  
    syscall_name[200] = syscall_array[74];  
    syscall_name[201] = syscall_array[75];  
    syscall_name[202] = syscall_array[76];  
    syscall_name[203] = syscall_array[77];  
    syscall_name[204] = syscall_array[78];  
    syscall_name[206] = syscall_array[79];  
    syscall_name[207] = syscall_array[80];  
    syscall_name[208] = syscall_array[81];  
    syscall_name[216] = syscall_array[82];  
    syscall_name[226] = syscall_array[83];  
    syscall_name[233] = syscall_array[84];  
    syscall_name[261] = syscall_array[85];
    syscall_name[253] = syscall_array[86];
    syscall_name[283] = syscall_array[87];
    syscall_name[ 48] = syscall_array[88];
    syscall_name[ 71] = syscall_array[89];
    syscall_name[ 72] = syscall_array[90];
    syscall_name[ 78] = syscall_array[91];
    syscall_name[ 82] = syscall_array[92];
    syscall_name[103] = syscall_array[93];
    syscall_name[116] = syscall_array[94];
    syscall_name[131] = syscall_array[95];
    syscall_name[165] = syscall_array[96];
    syscall_name[166] = syscall_array[97];
    syscall_name[174] = syscall_array[98];
    syscall_name[176] = syscall_array[99];
    syscall_name[227] = syscall_array[100];
    syscall_name[276] = syscall_array[101];
    syscall_name[254] = syscall_array[102];

}