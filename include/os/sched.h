/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
#include <context.h>
#include <pgtable.h>
#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/system.h>
#include <os/lock.h>
#include <os/elf.h>
#include <os/source.h>
#include <fs/file.h>
#include <os/signal.h>
#include <os/resource.h>
#include <user_programs.h>
#include <sys/special_ctx.h>

#define NUM_MAX_TASK 100
#define NUM_MAX_USTACK 2
// #define PRIORITY
#define NO_PRIORITY
#define NCPU 2

#define CLONE_PARENT_SETTID	    0x00100000
#define CLONE_CHILD_CLEARTID	0x00200000
#define CLONE_CHILD_SETTID	    0x01000000
#define CLONE_VM	            0x00000100
#define CLONE_FILES	            0x00000400
#define CLONE_SIGHAND	        0x00000800
#define CLONE_SETTLS            0x00080000
extern void ret_from_exception();

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

#define FAST
/* Process Control Block */
#pragma pack(8)
typedef struct pcb
{
    /* register context */  
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    /* PGDIR */
    PTE pgdir;

    /*handle signal*/
    uint64_t is_handle_signal;

    /* kernal stack base */
    ptr_t kernel_stack_top;
    ptr_t kernel_stack_base;
    /* user stack base */
    ptr_t user_stack_top;
    ptr_t user_stack_base;

    /* can be used */
    uint64_t used;

    /* name for debug */
    char name[50];
    /* list */
    list_node_t list;

    list_head wait_list;

    
    regs_context_t *save_context;
    switchto_context_t *switch_context;
    
    /* pcb id */
    pid_t pid;
    pid_t ppid;
    pid_t tid;

    /*tid control*/
    uint32_t *clear_ctid;

    /* point to parent */
    struct parent{
        struct pcb *parent;
        uint32_t flag;
    }parent;
    
    /* USER | KERNEL (THREAD/PROCESS) */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    /* exit status */
    int exit_status;

    // the flag
    uint32_t flag;

    spawn_mode_t mode;

    /* sleep time */
    uint64_t end_time;

    /* systime */
    uint64_t stime;
    uint64_t utime;
    /* the elf message */
    ELF_info_t elf;
    /* fs */
    fd_t *pfd;
    struct rlimit fd_limit;
    // fd_t *pfd;
    /* mask used for taskset */
    uint32_t mask;
    /* rlimit */
    
    // for the special pipe
    ctx_pipe_t * ctx_pipe_array;

    /* priority */
    uint64_t priority;

    /* edata */
    uint64_t edata;   

    /* last run */
    uint64_t pre_time;

    /* the number of physic_page, no more than 3 */
    uint32_t pge_num;
    #ifdef FAST
        /* for the fast load */ 
        excellent_load_t *exe_load;
    #endif
    /* stand the execve */
    uint32_t execve;

    /* signal handler */
    sigaction_t *sigactions;
    uint64_t sig_recv;
    uint64_t sig_pend;
    uint64_t sig_mask;
    uint64_t prev_mask;

    /* itimer */
    timer_t itimer;
} pcb_t;
#pragma pack()
// exited
#define WEXITSTATUS(exit_status) ((exit_status) << 8) & 0xff00)
// conv-default
static const char *fixed_envp[] = {
                                    "SHELL=/bin/bash",
                                    "PWD=/",
                                    "LOGNAME=root",
                                    "MOTD_SHOWN=pam",
                                    "HOME=/root",
                                    "LANG=C.UTF-8",
                                    "INVOCATION_ID=9f58417fca9d46d4a23cde1329404868",
                                    "TERM=vt220",
                                    "USER=root",
                                    "SHLVL=1",
                                    "JOURNAL_STREAM=8:9290",
                                    "HUSHLOGIN=FALSE",
                                    "PATH=.",
                                    "MAIL=/var/mail/root",
                                    "_=/usr/bin/bash",
                                    "OLDPWD=/root",
                                    "LD_LIBRARY_PATH=.",
                                    #ifndef k210
                                        "ENOUGH=5000",
                                        "TIMING_O=12",
                                        "LOOP_O=0.0027",
                                    #endif
                                    NULL
                                   };


/* signal bugger */ 
extern regs_context_t signal_context_buf;
extern switchto_context_t signal_switch_buf;

/* signal share */
typedef struct sigaction_table{
    int used; 
    int num; // the number of process;
    sigaction_t sigactions[NUM_SIG];
}sigaction_table_t;

/* ready queue to run */
extern list_head ready_queue;
/* zombie_queue */
extern list_head zombie_queue;
/* used pcb queue */
extern list_head used_queue;

/* avaliable pcb queue */
extern source_manager_t available_queue;

// special
extern char test_name[50];

/* smp */
pcb_t * volatile current_running_master;
pcb_t * volatile current_running_slave;

extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
// static pcb_t pcb[NUM_MAX_TASK];
// static LIST_HEAD(ready_queue);
// static LIST_HEAD(ZOMBIE_queue);

/*master*/
extern volatile pcb_t pid0_pcb_master;
extern const ptr_t pid0_stack_m;
/*slave*/
extern volatile pcb_t pid0_pcb_slave;
extern const ptr_t pid0_stack_s;

// gp
extern void __global_pointer$();


/* init */
void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[]);
void init_clone_stack(pcb_t *pcb, void * tls);

// load process mirroring
uintptr_t load_process_memory(const char * path, pcb_t * initpcb);

/* init the pab struct member */
void init_pcb_member(pcb_t * initpcb,  task_type_t type, spawn_mode_t mode);
/* put the argc and argv in the user stack */
void set_argc_argv(int argc, char * argv[], pcb_t *init_pcb);
/* put the argc and argv in the user stack for std riscv */
void set_argc_argv_std(int argc, char * argv[], pcb_t *init_pcb);
/* init pcb clone */
void init_clone_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode);
/* init execve pcb */
void init_execve_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode);
/* clone copy on write*/
void copy_on_write(PTE src_pgdir, PTE dst_pgdir);
/* handle exec for pipe and redirect */
int handle_exec_pipe_redirect(pcb_t *initpcb, pid_t *pipe_pid, int argc, char* argv[]);

/* copy all the things on the user stack */ 
uintptr_t copy_thing_user_stack(pcb_t *init_pcb, ptr_t user_stack, int argc, \
            char *argv[], char *envp[], char *filenam);


/* scheduler */
extern void switch_to(pcb_t *prev, pcb_t *next);
extern void do_scheduler(void);
void do_priori(int,int);

pcb_t * get_higher_process(list_head *,uint64_t);
int get_file_from_kernel(const char * elf_name);

/* recyclre the pcb */
int recycle_pcb_default(pcb_t *);


pcb_t *get_free_pcb();
void free_pcb(pcb_t *RecyclePcb);
void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);                

/* process */
extern pid_t do_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
extern pid_t do_execve(const char* path, char* argv[], char *const envp[]);
extern void do_exit(int32_t exit_status);
extern void do_exit_group(int32_t exit_status);
extern uint64_t do_wait4(pid_t pid, uint16_t *status, int32_t options);
uint64_t get_num_from_parm(char *parm[]);

/* id */
extern pid_t do_getpid();
extern pid_t do_getppid();
extern pid_t do_set_tid_address(int *tidptr);
pid_t do_setsid(void);
uid_t do_geteuid(void);
gid_t do_getegid(void);
pid_t do_gettid(void);
gid_t do_getgid(void);
uid_t do_getuid(void);
/* clone */
extern pid_t do_clone(uint32_t flag, uint64_t stack, pid_t ptid, void *tls, pid_t ctid);

/* signal */
void change_signal_mask(pcb_t *pcb, uint64_t newmask);  
void trans_pended_signal_to_recv(pcb_t *pcb);
void send_signal(int signum, pcb_t *pcb);

/* exit */
void handle_exit_pcb(pcb_t * exitPcb);
void release_wait_deal_son(pcb_t * exitPcb);

/* final competition */
#define __CPU_SETSIZE	1024
typedef unsigned long int __cpu_mask;
#define __NCPUBITS	(8 * sizeof (__cpu_mask))
struct sched_param
{
  int sched_priority;
};

typedef struct
{
  unsigned long int __bits[__CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

extern int do_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
extern int do_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
// kill
extern int32_t do_kill(pid_t pid, int32_t sig);
extern int32_t do_tkill(int tid, int sig);
extern int32_t do_tgkill(int tgid, int tid, int sig);
#endif
