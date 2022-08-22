/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/syscall.h>
#include <os/uname.h>
#include <os/lock.h>
#include <os/system.h>
#include <os/signal.h>
#include <os/futex.h>
#include <os/socket.h>
#include <os/smp.h>
#include <os/io.h>
#include <sys/special_ctx.h>
#include <fs/poll.h>
#include <fs/pipe.h>
// #include <csr.h>
#include <pgtable.h>
#include <sdcard.h>
#include <sbi.h>
#include <sbi_rust.h>
#include <stdio.h>
#include <screen.h>
#include <common.h>
#include <assert.h>

int cancel_flag_m = 0;
int cancel_flag_s = 0;

// init available pcb
static void init_pcb_queue()
{
    init_list(&available_queue.free_source_list);
    init_list(&available_queue.wait_list);
    // init all pcb available queue
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        pcb[i].used = 0;
        pcb[i].status = TASK_EXITED;
        list_add_tail(&pcb[i].list, &available_queue.free_source_list);
    }
    available_queue.source_num = NUM_MAX_TASK - 1;
}

// init the first process - shell
static void init_shell()
{
    /* alloc the first_level page */
    pcb[0].pgdir = kmalloc(PAGE_SIZE);
    pcb[0].kernel_stack_top = allocPage();         //a kernel virtual addr, has been mapped
    pcb[0].user_stack_top = USER_STACK_ADDR;       //a user virtual addr, not mapped
    share_pgtable(pcb[0].pgdir, pa2kva(PGDIR_PA));
    /* map the user_stack */
    alloc_page_helper(pcb[0].user_stack_top - PAGE_SIZE, pcb[0].pgdir, MAP_USER);
    /* init other pcb */
    // for (int i = 1; i < NUM_MAX_TASK; i++){
    //     pcb[i].used = 0;
    // }
    init_pcb_queue();

    kstrcpy(pcb[0].name, elf_files[0].file_name);    
    /* init pcb member */
    init_pcb_member(&pcb[0], USER_PROCESS, ENTER_ZOMBIE_ON_EXIT);
    // init load elf
    init_pre_load();
    pcb[0].mask = 3;
    pcb[0].priority = 3;
    /* copy the name */


    uint64_t length = 0;
    uint32_t elf_id = get_file_from_kernel("shell");
    ptr_t entry_point = load_elf(elf_files[elf_id].file_content,
                                *elf_files[elf_id].file_length, 
                                pcb[elf_id].pgdir,
                                &length, 
                                &pcb[0],
                                alloc_page_helper);
    /* init stack */
    init_pcb_stack(pcb[0].kernel_stack_top, pcb[0].user_sp, entry_point, &pcb[0], NULL, NULL);
    /* add it to ready queue */
    list_add(&pcb[0].list,&ready_queue);

    init_list(&pid0_pcb_master.list);
    init_list(&pid0_pcb_slave.list);
    pid0_pcb_master.end_time = 0;
    pid0_pcb_master.utime = 0;
    pid0_pcb_master.stime = 0;
    pid0_pcb_slave.end_time = 0;
    pid0_pcb_slave.utime = 0;
    pid0_pcb_slave.stime = 0;
    sys_time_master = sys_time_slave = get_ticks();
    current_running_master = &pid0_pcb_master;
    current_running_slave  = &pid0_pcb_slave; 
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
	for(i = 0; i < NUM_SYSCALLS; i++)
		syscall[i] = (long int (*)()) &handle_miss;
// ============ for the shell===================
    /* tmpt */ 
    syscall[SYS_screen_write]           = (long int (*)())&screen_write;   
    syscall[SYS_screen_reflush]         = (long int (*)())&screen_reflush;
    syscall[SYS_screen_clear]           = (long int (*)())&screen_clear;
    syscall[SYS_get_timer]              = (long int (*)())&get_timer;         
    syscall[SYS_get_tick]               = (long int (*)())&get_ticks;
    syscall[SYS_exec]                   = (long int (*)())&do_exec;
    syscall[SYS_shm_pageget]            = (long int (*)())&shm_page_get;
    syscall[SYS_shm_pagedt]             = (long int (*)())&shm_page_dt; 
    syscall[SYS_disk_write]             = (long int (*)())&disk_write;
    syscall[SYS_disk_read]              = (long int (*)())&disk_read;
    syscall[SYS_preload]                = (long int (*)())&do_pre_load;
    syscall[SYS_extend]                 = (long int (*)())&do_extend;
// =========== for the k210===================
    /* id */
    syscall[SYS_getpid]              = (long int (*)())&do_getpid; 
    syscall[SYS_getppid]             = (long int (*)())&do_getppid;

    /* time */
    syscall[SYS_gettimeofday]        = (long int (*)())&do_gettimeofday;
    syscall[SYS_times]               = (long int (*)())&do_gettimes;

    /* process */
    syscall[SYS_exit]                = (long int (*)())&do_exit;
    syscall[SYS_clone]               = (long int (*)())&do_clone;
    syscall[SYS_wait4]               = (long int (*)())&do_wait4;
    syscall[SYS_nanosleep]           = (long int (*)())&do_nanosleep;
    syscall[SYS_execve]              = (long int (*)())&do_execve;
    syscall[SYS_sched_yield]         = (long int (*)())&do_scheduler;
    /* id */
    /* sys_msg */
    syscall[SYS_uname]               = (long int (*)())&do_uname;

    /* mm */
    syscall[SYS_brk]                 = (long int (*)())&do_brk;
    // syscall[SYS_brk]                 = (long int (*)())&lazy_brk;


    /* fat32 */
    /* open close */
    syscall[SYS_openat]              = (long int (*)())&fat32_openat;
    syscall[SYS_close]               = (long int (*)())&fat32_close;

    /* read */
    syscall[SYS_read]                = (long int (*)())&fat32_read;
    // /* write */
    syscall[SYS_write]               = (long int (*)())&fat32_write;

    syscall[SYS_pipe2]               = (long int (*)())&fat32_pipe2;
    syscall[SYS_dup]                 = (long int (*)())&fat32_dup;
    syscall[SYS_dup3]                = (long int (*)())&fat32_dup3;

    syscall[SYS_getcwd]              = (long int (*)())&fat32_getcwd;
    syscall[SYS_chdir]               = (long int (*)())&fat32_chdir;
    syscall[SYS_mkdirat]             = (long int (*)())&fat32_mkdirat;
    syscall[SYS_getdents64]          = (long int (*)())&fat32_getdents;
    syscall[SYS_linkat]              = (long int (*)())&fat32_link;
    syscall[SYS_unlinkat]            = (long int (*)())&fat32_unlink;
    syscall[SYS_mount]               = (long int (*)())&fat32_mount;
    syscall[SYS_umount2]             = (long int (*)())&fat32_umount;
    syscall[SYS_fstat]               = (long int (*)())&fat32_fstat;
    syscall[SYS_mmap]                = (long int (*)())&fat32_mmap;
    syscall[SYS_munmap]              = (long int (*)())&fat32_munmap;
/* == final comp == */
// stage 1
    syscall[SYS_fcntl]               = (long int (*)())&fat32_fcntl;
    syscall[SYS_ioctl]               = (long int (*)())&do_ioctl;
    syscall[SYS_readv]               = (long int (*)())&fat32_readv;
    syscall[SYS_writev]              = (long int (*)())&fat32_writev;
    syscall[SYS_pread]               = (long int (*)())&fat32_pread;
    syscall[SYS_pwrite]              = (long int (*)())&fat32_pwrite;
    syscall[SYS_lseek]               = (long int (*)())&fat32_lseek;
    syscall[SYS_fstatat]             = (long int (*)())&fat32_fstatat;
    syscall[SYS_utimensat]           = (long int (*)())&fat32_utimensat;
    syscall[SYS_exit_group]          = (long int (*)())&do_exit_group;
    syscall[SYS_set_tid_address]     = (long int (*)())&do_set_tid_address;
    syscall[SYS_clock_gettime]       = (long int (*)())&do_clock_gettime;
    // id
    syscall[SYS_geteuid]             = (long int (*)())&do_geteuid;
    syscall[SYS_getegid]             = (long int (*)())&do_getegid;
    syscall[SYS_gettid]              = (long int (*)())&do_gettid;
    syscall[SYS_setsid]              = (long int (*)())&do_setsid;
    
    syscall[SYS_sysinfo]             = (long int (*)())&do_sysinfo;
    syscall[SYS_statfs]              = (long int (*)())&fat32_statfs;
    syscall[SYS_fstatfs]             = (long int (*)())&fat32_fstatfs;
    syscall[SYS_getrlimit]           = (long int (*)())&do_getrlimit;
    syscall[SYS_setrlimit]           = (long int (*)())&do_setrlimit;
    syscall[SYS_prlimit]             = (long int (*)())&do_prlimit;
    syscall[SYS_tkill]               = (long int (*)())&do_tkill;
    syscall[SYS_sched_setscheduler]  = (long int (*)())&do_sched_setscheduler;
    syscall[SYS_sched_getaffinity]   = (long int (*)())&do_sched_getaffinity;
    syscall[SYS_ppoll]               = (long int (*)())&do_ppoll;  
    syscall[SYS_kill]                = (long int (*)())&do_kill;
    syscall[SYS_rt_sigaction]        = (long int (*)())&do_rt_sigaction;
    syscall[SYS_rt_sigprocmask]      = (long int (*)())&do_rt_sigprocmask;
    syscall[SYS_rt_sigreturn]        = (long int (*)())&do_rt_sigreturn;
    syscall[SYS_rt_sigtimedwait]     = (long int (*)())&do_rt_sigtimedwait;
    syscall[SYS_mprotect]            = (long int (*)())&do_mprotect;
    syscall[SYS_futex]               = (long int (*)())&do_futex;
    syscall[SYS_set_robust_list]     = (long int (*)())&do_set_robust_list;
    syscall[SYS_get_robust_list]     = (long int (*)())&do_get_robust_list;
    syscall[SYS_socket]              = (long int (*)())&do_socket;
    syscall[SYS_bind]                = (long int (*)())&do_bind;
    syscall[SYS_listen]              = (long int (*)())&do_listen;
    syscall[SYS_connect]             = (long int (*)())&do_connect;
    syscall[SYS_accept]              = (long int (*)())&do_accept;
    syscall[SYS_getsockname]         = (long int (*)())&do_getsockname;
    syscall[SYS_setsockopt]          = (long int (*)())&do_setsockopt;
    syscall[SYS_sendto]              = (long int (*)())&do_sendto;
    syscall[SYS_recvfrom]            = (long int (*)())&do_recvfrom;
    syscall[SYS_mremap]              = (long int (*)())&fat32_mremap;
    syscall[SYS_madvise]             = (long int (*)())&do_madvise;
    syscall[SYS_membarrier]          = (long int (*)())&do_membarrier;
//stage 2
    syscall[SYS_tgkill]              = (long int (*)())&do_tgkill;
    syscall[SYS_getuid]              = (long int (*)())&do_getuid;
    syscall[SYS_getgid]              = (long int (*)())&do_getgid;
    syscall[SYS_faccessat]           = (long int (*)())&fat32_faccessat;
    syscall[SYS_fsync]               = (long int (*)())&fat32_fsync;
    syscall[SYS_syslog]              = (long int (*)())&do_syslog;
    syscall[SYS_vm86]                = (long int (*)())&do_vm86;
    syscall[SYS_readlinkat]          = (long int (*)())&fat32_readlinkat; // specialized
    // below TODO
    syscall[SYS_getrusage]           = (long int (*)())&do_getrusage; 
    syscall[SYS_sendfile]            = (long int (*)())&fat32_sendfile;
    syscall[SYS_pselect6]            = (long int (*)())&do_pselect6;
    syscall[SYS_setitimer]           = (long int (*)())&do_setitimer;
    syscall[SYS_msync]               = (long int (*)())&fat32_msync;
    syscall[SYS_renameat2]           = (long int (*)())&fat32_renameat2;
}
/**
 * @brief preprocessing SD, unuseful but need;
 * 
 */
void sd_preprocessing(){ 
    uint8_t A[512];
    // uint8_t B[512];
    int i;
    sd_read_sector(A, FAT_FIRST_SEC, 1);
    if(kmemcmp((char*)(A + 82), "FAT32", 5)){
        printk("> [Preprocessing] tmp failed for the init FAT32\n");
    }    
}

/**
 * @brief cancel the boot map
 * 
 */
void cancel_kernel_map(){
    // null
    uint64_t vpn2 = 
        BOOT_KERNEL >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    PTE *early_pgdir = pa2kva((PTE *)PGDIR_PA);
    early_pgdir[vpn2] = 0;
    local_flush_tlb_all();
}

// The beginning of everything >_< ~~~~~~~~~~~~~~
int main(unsigned long mhartid)
{   
    if (!mhartid) {
        printk("> [INIT] Core 0 enter kernel\n\r");
        // init kernel lock
        init_kernel_lock();

        // init mem
        uint32_t free_page_num = init_mem();
        printk("> [INIT] %d Mem initialization succeeded.\n\r", free_page_num); 
    
        // wakeup_other_hart
        for (int i = 1; i < NCPU; i ++) {
           uint64_t mask = 1 << i;
        // ===== for 0.2.2 rust_sbi ======
        // #ifndef k210
        //    wakeup_other_hart();
        // #else 
        //    struct sbiret ipi = sbi_send_ipi_k210(mask, 0);
        // #endif
        // ===== for 0.2.2 rust_sbi ======
            wakeup_other_hart();
	    }
        while (!cancel_flag_m);

        // cancel_kernel_map();
        // init fd_table
        init_fd_table();
        init_sig_table();
        printk("> [INIT] fd and sig table initialization succeeded.\n\r");
        // init shell
        init_shell();
        pid0_pcb_master.save_context->core_id = 0;

        time_base = TIMEBASE;
        // time_base = TIMEBASE;

        printk("> [INIT] Shell initialization succeeded.\n\r"); 
        binit();
        printk("> [INIT] bio cache initialization succeeded.\n\r"); 
        // init disk
        #ifdef k210
            fpioa_pin_init();
            printk("> [INIT] fpioa_pin init succeeded.\n\r");
            disk_init();
            sd_preprocessing();

            //sd_performance_test();
            //while(1);
            printk("> [INIT] sdcard init succeeded.\n\r");
        #endif
        fat32_init();
        printk("> [INIT] FAT32 init succeeded.\n\r");
        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");   
        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");
        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");
        // init futex
        init_system_futex();
        printk("> [INIT] System_futex initialization succeeded.\n\r");
        // init socket
        init_socket();
        printk("> [INIT] Socket initialization succeeded.\n\r");
        cancel_flag_s = 1;
        // while(1);
        uint64_t p = 0;
    }else{
        printk("> [INIT] Core 1 enter kernel\n");
        cancel_flag_m = 1;
        // wait clean boot map
        while(!cancel_flag_s);
        uint64_t p = 0;
        pid0_pcb_slave.save_context->core_id = 1;
        setup_exception();  
        while(1);    
    }
    // printk("ticks: %d\n", get_ticks());
    sbi_set_timer(get_ticks() + time_base/50);
    // open float
    enable_float_point_inst();    
    /* open interrupt */
    enable_interrupt();
 
    while (1) {    
        __asm__ __volatile__("wfi\n":::);
    }
    return 0;
}
