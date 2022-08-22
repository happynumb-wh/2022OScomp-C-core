#include <os/list.h>
#include <screen.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <os/spinlock.h>
#include <os/string.h>
#include <swap/swap.h>
#include <fs/fat32.h>
#include <stdio.h>
#include <assert.h>
#include <os/futex.h>

#define PRIORIT_BASE 4
#define TIME_BASE 1
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_MSTER - \
                           sizeof(regs_context_t) - \
                           sizeof(switchto_context_t);//0x0e70;
volatile pcb_t pid0_pcb_master = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .pgdir = PGDIR_PA,
    .preempt_count = 0,
    .name = "pid0_master",
    .save_context = (regs_context_t *)(INIT_KERNEL_STACK_MSTER - sizeof(regs_context_t)),
    .switch_context = (switchto_context_t *)(INIT_KERNEL_STACK_MSTER - \
                        sizeof(regs_context_t) - \
                        sizeof(switchto_context_t)),
    .status   = TASK_READY,
    .type = KERNEL_PROCESS
};  //master
const ptr_t pid0_stack_s = INIT_KERNEL_STACK_SLAVE - \
                           sizeof(regs_context_t) - \
                           sizeof(switchto_context_t);//0x0e70;
volatile pcb_t pid0_pcb_slave = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .pgdir = PGDIR_PA,    
    .preempt_count = 0,
    .name = "pid0_slave",
    .save_context = (regs_context_t *)(INIT_KERNEL_STACK_SLAVE - sizeof(regs_context_t)),
    .switch_context = (switchto_context_t *)(INIT_KERNEL_STACK_SLAVE - \
                        sizeof(regs_context_t) - \
                        sizeof(switchto_context_t)),
    .status   = TASK_READY,
    .type = KERNEL_PROCESS
};  //slave

LIST_HEAD(ready_queue);
LIST_HEAD(zombie_queue);
LIST_HEAD(used_queue)
// available pcb queue
source_manager_t available_queue;

// for special
char test_name[50] = {0};
int record = 0;

/* current running task PCB */
// pcb_t * volatile current_running;
// pcb_t * volatile prev_running;
/* global process id */
pid_t process_id = 1;

/* 优先级调度 */
pcb_t * get_higher_process(list_head * queue, uint64_t schedler_time){  
    int score = 0;
    list_node_t * pre;
    pcb_t * get_pcb;
    for(pre = queue->prev; pre!=queue && !list_empty(queue) ; pre=pre->prev ){
        pcb_t * check = list_entry(pre, pcb_t, list);
        int get_score = check->priority * PRIORIT_BASE + ((schedler_time-check->pre_time) / 6000)*TIME_BASE;
        if(get_score > score){
            get_pcb = check;
            score = get_score;
        } 
    }
    return get_pcb;
}

void do_scheduler(void)
{
    // TODO schedule
    
    uint64_t cpu_id = get_current_cpu_id();
    pcb_t * current_running = cpu_id == 0 ? current_running_master : current_running_slave; 
    // printk("entry name: %s, pid:%d, the sepc: 0x%lx, the ra: %lx, addr: %lx\n", current_running->name,\
    //     current_running->pid, current_running->save_context->sepc,current_running->switch_context->regs[0], current_running);
    pcb_t * prev_running = current_running;  
    // if (list_empty(&ready_queue)) goto end;
    if (current_running->status == TASK_RUNNING && current_running->pid) {
        current_running->status = TASK_READY;
        list_add_tail(&current_running->list, &ready_queue);
    }
    /* priority */
    #ifdef PRIORITY
        uint64_t now = get_ticks();
        current_running->pre_time = now; 
        current_running = get_higher_process(&ready_queue,now);
        list_del(&current_running->list);
    #endif
    #ifdef NO_PRIORITY
        if (!list_empty(&ready_queue)) { 
            current_running = list_entry(ready_queue.next, pcb_t, list);
            list_del(ready_queue.next);
            if (cpu_id)
                current_running_slave  = current_running;
            else 
                current_running_master = current_running;
        }else{
            if (current_running->status != TASK_RUNNING) {
                current_running = cpu_id == 0 ? &pid0_pcb_master : &pid0_pcb_slave;
                if(cpu_id == 0)
                    current_running_master = current_running;
                else
                    current_running_slave  = current_running;
            }
        }
    #endif
    // release the lock
    if(get_pgdir() == kva2pa(current_running->pgdir)) ;
    else{
        set_satp(SATP_MODE_SV39, current_running->pid, \ 
                (uint64_t)kva2pa((current_running->pgdir)) >> 12);
        local_flush_tlb_all();
    }
    // save the id
    current_running->save_context->core_id = \
        prev_running->save_context->core_id;
    // TODO: switch_to current_running
    current_running->status = TASK_RUNNING;
    // printk("next name: %s, pid:%d, the sepc: 0x%lx, the ra: %lx, addr: %lx\n", current_running->name,\
    //     current_running->pid, current_running->save_context->sepc, current_running->switch_context->regs[0], current_running);
    if (current_running->execve)
    {
        current_running->execve = 0;
        ret_from_exception();
    }
    switch_to(prev_running, current_running);  
}


void do_priori(int priori, int pcb_id){
    if(pcb_id){
        pcb[pcb_id].priority = priori;
        pcb[pcb_id].status = TASK_READY;
        pcb[pcb_id].pre_time = get_ticks() ;
        list_add(&pcb[pcb_id].list, &ready_queue);
    }      
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // if the process was killed, should kill 
    handle_signal();
    // TODO: block the pcb task into the block queue
    pcb_t * current_running = get_current_running();
    // printk("block pid:%d\n", current_running->pid);
    list_add(pcb_node,queue);
    current_running->status = TASK_BLOCKED;
    do_scheduler();
    // printk("unblock pid:%d\n", current_running->pid);
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    list_del(pcb_node);
    list_entry(pcb_node , pcb_t, list)->status=TASK_READY;
    list_add(pcb_node, &ready_queue);
}


int get_file_from_kernel(const char * elf_name){
    int file_id;
    for (file_id = 0; file_id < ELF_FILE_NUM; file_id++){
        if (!kstrcmp(elf_name, elf_files[file_id].file_name)){
            break;
        }       
    }    
    if (file_id == ELF_FILE_NUM)
        return -1;
    return file_id;
}


/**
 * @brief exec one process, used by the shell
 * @return succeed return the new pid failed return -1
 */
pid_t do_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    // printk("enter do_exec\n");
    // we will always get one pcb
    // printk("[exec] %s\n", file_name);
    printk("I have %d pages\n", freePageManager.source_num);
    pcb_t *initpcb = get_free_pcb();
    pcb_t *current_running = get_current_running();
    if (argc >= 2)
        kstrcpy(test_name, argv[1]);
    // init the pcb 
    initpcb->pgdir = allocPage() - PAGE_SIZE;   
    initpcb->kernel_stack_top = allocPage();         //a kernel virtual addr, has been mapped
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->user_stack_top = USER_STACK_ADDR;       //a user virtual addr, not mapped
    share_pgtable(initpcb->pgdir, pa2kva(PGDIR_PA));
    /* map the user_stack, for NUM_MAX_USTACK*/
    for (int i = 0; i < NUM_MAX_USTACK; i++)
    {
        alloc_page_helper(initpcb->user_stack_top - (i + 1) * PAGE_SIZE, initpcb->pgdir, MAP_USER);
    }
    
    /* copy the name */
    kstrcpy(initpcb->name, file_name); 
    /* init pcb mem */
    init_pcb_member(initpcb, USER_PROCESS, AUTO_CLEANUP_ON_EXIT);
    initpcb->parent.parent = get_current_running();
    initpcb->ppid = initpcb->parent.parent->pid;
    initpcb->mask = 3;
    initpcb->priority = 3;
    // load process into memory
    ptr_t entry_point = load_process_memory(file_name, initpcb);

    // dynamic
    if (initpcb->exe_load->dynamic)
    {
        entry_point = load_connector("libc.so", initpcb->pgdir);
    }

    if (entry_point == 0)
    {
        printk("[Error] load %3s to memory false\n", file_name);
        return -1;
    }
    pid_t pipe_pid = 0;
    // handle for pipe and redirect
    int true_argc = handle_exec_pipe_redirect(initpcb, &pipe_pid, argc, argv);

    // copy all things on the user stack
    initpcb->user_sp = copy_thing_user_stack(initpcb, initpcb->user_stack_top, true_argc, \
                        argv, fixed_envp, file_name);

    initpcb->user_stack_top = initpcb->user_sp;
    // init all stack
    init_pcb_stack(initpcb->kernel_sp, initpcb->user_sp, entry_point, initpcb, \
                    true_argc, argv);
    // add to ready queue
    list_add(&initpcb->list, &ready_queue);
    // printk("[exec] %lx\n", entry_point);
    return pipe_pid ? pipe_pid : initpcb->pid;
}

/**
 * @brief execve one process by current pcb
 * @param file_name file name
 * @param argv argv
 * @param envp envp
 * @return succeed never return, failed retuen -1
 * 
 */
pid_t do_execve(const char* path, char* argv[], char *const envp[]){
    // printk("enter exec\n");
    // printk("[execve] %s\n", path);
    pcb_t * initpcb = get_current_running();
    uintptr_t pre_pgdir = initpcb->pgdir;
    // a new pgdir
    initpcb->pgdir = allocPage() - PAGE_SIZE; 
    uint64_t pre_edata = initpcb->edata;

    initpcb->user_stack_top = USER_STACK_ADDR;       //a user virtual addr, not mapped
    initpcb->kernel_sp =initpcb->kernel_stack_top;
    share_pgtable(initpcb->pgdir, pa2kva(PGDIR_PA));
    
    /* map the user_stack */
    /* map the user_stack, for NUM_MAX_USTACK*/
    for (int i = 0; i < NUM_MAX_USTACK; i++)
    {
        alloc_page_helper(initpcb->user_stack_top - (i + 1) * PAGE_SIZE, initpcb->pgdir, MAP_USER);
    }
    
    excellent_load_t *pre_exe_load = initpcb->exe_load;

    /* handle ./xxx */
    for (int i = 0; i < kstrlen(path); i++){
        if(path[i] == '/'){
            path = path + i + 1;
            break;
        }
    }
    
    /* copy the name */ 
    kstrcpy(initpcb->name, path); 

    /* init pcb mem */
    init_execve_pcb(initpcb, USER_PROCESS, AUTO_CLEANUP_ON_EXIT);
    
    initpcb->mask = 3;
    initpcb->priority = 3;
    // load process into memory
    ptr_t entry_point = load_process_memory(path, initpcb);

    if (entry_point == 0)
    {
        printk("[Error] load %s to memory false]\n", path);
        return -1;
    }

    // dynamic
    if (initpcb->exe_load->dynamic)
    {
        entry_point = load_connector("libc.so", initpcb->pgdir);
    }

    int argc = (int)get_num_from_parm(argv);

    // copya all things on the user stack
    initpcb->user_sp = copy_thing_user_stack(initpcb, initpcb->user_stack_top, argc, \
                        argv, envp, path);

    initpcb->user_stack_top = initpcb->user_sp;
    uint64_t save_core_id = get_current_cpu_id();
    // initialize process
    init_pcb_stack(initpcb->kernel_sp, initpcb->user_sp, entry_point, \
                    initpcb, argc, argv);
    initpcb->save_context->core_id = save_core_id;

    // add in the ready_queue
    list_add(&initpcb->list, &ready_queue);
    #ifdef FAST
        recycle_page_part(pre_pgdir, pre_exe_load, pre_edata);
    #else
        recycle_page_voilent(pre_pgdir);
    #endif

    // no signal
    do_scheduler();
    // this never return
    return initpcb->pid;    
}


/**
 * @brief wait the process if pid == -1, 
 * wait every child, else wait the pointed process
 * @param pid the wait pid
 * @param status the status when wnter exit
 * @param option not care now
 * @return true return the wait pid, else return -1
 */
uint64_t do_wait4(pid_t pid, uint16_t *status, int32_t options){
    pcb_t * current_running = get_current_running();
    pid_t ret = -1;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].parent.parent == current_running && (pid == -1 || pid == pcb[i].pid)) {
            if (pcb[i].status != TASK_EXITED && pcb[i].status != TASK_ZOMBIE) {
                ret = pcb[i].pid;
                do_block(&current_running->list, &pcb[i].wait_list);
                /* set status */
                if(status){
                    handle_page_fault_store(NULL, status, NULL);
                    (*((uint16_t *)status) = WEXITSTATUS(pcb[i].exit_status); 
                }
                // exit it 
                if (pcb[i].status == TASK_ZOMBIE)
                    handle_exit_pcb(&pcb[i]);
                // WEXITSTATUS(get_kva_of(status, current_running->pgdir), pcb[i].exit_status);
            } else if (pcb[i].status == TASK_ZOMBIE)
            {
                ret = pcb[i].pid;
                /* recycle pcb */
                // seat status
                if(status){
                    handle_page_fault_store(NULL, status, NULL);
                    (*((uint16_t *)status) = WEXITSTATUS(pcb[i].exit_status);
                }
                // handle exit
                handle_exit_pcb(&pcb[i]);
            }  
        }
    }
    // printk("[wait4] pid = %d wakeup after exit of %d, status = %d\n",current_running->pid,pid,*(int16_t *)status);
    return ret;   
}

/**
 * @brief pcb exit myself we will recycle all the source now
 * for the moment
 * @param exit_status : exit status
 * @return no return 
 * 
 */
void do_exit(int32_t exit_status){
    pcb_t * current_running = get_current_running();
    pcb_t * exit_pcb;
    exit_pcb = current_running;
    // printk("exit process pid = %d\n",current_running->pid);
    pcb_t *wait_entry = NULL, *wait_q;

    /* exit status */
    current_running->exit_status = exit_status;

    if (current_running->clear_ctid) {
        // if (get_kva_of(current_running->clear_ctid,current_running->pgdir) == NULL) alloc_page_helper(current_running->clear_ctid,current_running->pgdir,MAP_USER);
        *(int *)(current_running->clear_ctid) = 0;
        do_futex(current_running->clear_ctid,FUTEX_WAKE,1,NULL,NULL,NULL);
    }
    
    if (current_running->mode == AUTO_CLEANUP_ON_EXIT) {
        /* free the wait */
        handle_exit_pcb(exit_pcb);
        // printk("Succeed recycle %d page from %s\n", recycle_page_num, exit_pcb->name);
    } else if (current_running->type == USER_PROCESS && \
               current_running->mode == ENTER_ZOMBIE_ON_EXIT) {
        current_running->status = TASK_ZOMBIE;
        if (current_running->flag & SIGCHLD)
        {
            // current_running->exit_status = 0;
            // printk("%s send signal to %s\n", current_running->name, current_running->parent.parent->name);
            send_signal(SIGCHLD, current_running->parent.parent);
        }        
    }
    // prints("%s has exit normally, exit_status: %lx\n", current_running->name, exit_status);
    /* scheduler */
    do_scheduler();  
}


void do_exit_group(int32_t exit_status){
    do_exit(exit_status);
}

/**
 * @brief clone one process,
 * succed return child's pid
 * failed return -1
 * ignore the flag, tls, ctid, pid
 */
pid_t do_clone(uint32_t flag, uint64_t stack, pid_t ptid, void *tls, pid_t ctid) 
{
    // printk("\t\tenter fork\n");
    pcb_t *current_running = get_current_running();
    pcb_t *initpcb = get_free_pcb();
    // printk("enter fork\n");
    /* alloc the pgdir */
    // for CONLE_VM, share the VM
    if (flag & CLONE_VM) initpcb->pgdir = current_running->pgdir;
    else
    {   
        initpcb->pgdir = allocPage() - PAGE_SIZE;
        /* copy kernel */
        share_pgtable(initpcb->pgdir, pa2kva(PGDIR_PA));         
    } 
    
    initpcb->kernel_stack_top = allocPage();         //a kernel virtual addr, has been mapped

    // if point usatck  
    initpcb->user_stack_top = stack ? stack : USER_STACK_ADDR;


    //init pfd_table
    initpcb->flag = flag;    

    // init pcb
    init_clone_pcb(initpcb, USER_PROCESS, ENTER_ZOMBIE_ON_EXIT);
    // don't need tmp
    // if (flag & CLONE_CHILD_SETTID) *(int *)ctid = initpcb->tid;
    // if (flag & CLONE_PARENT_SETTID) *(int *)ptid = initpcb->tid;
    // if (flag & CLONE_CHILD_CLEARTID) initpcb->clear_ctid = ctid;
    // for the qemu and k210, maybe we need to alloc the user stack first, 
    // don't care about the copy on write.
    // and we don't know whether it grows automatically
    // #ifndef k210
        uintptr_t ku_base;
        for (uintptr_t u_base = initpcb->user_stack_top - NORMAL_PAGE_SIZE; \
                (ku_base = get_kva_of(u_base, current_running->pgdir)) != NULL; \
                u_base -= NORMAL_PAGE_SIZE)
        {
            /* code */
            uintptr_t cloneu_base = alloc_page_helper(u_base, initpcb->pgdir, MAP_USER);

            if (status_ctx)
            {
                PTE * pte = get_PTE_of(u_base, current_running->pgdir);
                page_node_t * stack_page = &pageRecyc[GET_MEM_NODE(cloneu_base)];
                if (*pte & _PAGE_SD)
                {
                    *stack_page->page_pte = *pte;
                    // recyle it !
                    kfree_voilence(cloneu_base);
                    continue;
                }
            }

            share_pgtable(cloneu_base, ku_base);
        }
    // #endif    

    /* for COW clone */
    if ((flag & CLONE_VM) == 0)
        copy_on_write(current_running->pgdir, initpcb->pgdir);

    // the clone don't need new agv and envp, so we don't care
    // name
    kstrcpy(initpcb->name, current_running->name);
    kstrcat(initpcb->name, "_son");

    // init clone satck
    init_clone_stack(initpcb, tls);
    initpcb->user_stack_top = current_running->user_stack_top;
    
    list_add(&initpcb->list, &ready_queue);
    // printk("clone end\n");
    return initpcb->pid;
}

/* final competition */
int do_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param){
    printk("do_sched_setscheduler to do.\n");
    return 0;
}

int do_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask){
    //测试中只用了两次，按照该函数的功能，其类似函数setaffinity功能是把线程绑定
    //到一个CPU上，getaffinity的作用只是读出mask，测试中并未见到使用setaffinity
    //所以该函数直接return了
    printk("do_sched_getaffinity to do.\n");
    return 0;
}

int32_t do_kill(pid_t pid, int32_t sig){
    // printk("[kill] try to kill pid %d with sig %d\n",pid,sig);
    uint8_t ret = 0;
    pcb_t *current_running = get_current_running();
    for (uint32_t i = 0; i < NUM_MAX_TASK; i++){
        if ((pid > 0 && pcb[i].pid == pid) || (pid < -1 && pcb[i].pid == -pid)){
            send_signal(sig, &pcb[i]);
            ret++;
            break;
        }
        else if ((pid == 0) && !(current_running->status == TASK_EXITED)){
            send_signal(sig, current_running);
            ret++;
            break;
        }
        else if ((pid == -1) && !(pcb[i].status == TASK_EXITED)){
            send_signal(sig, &pcb[i]);
            ret++;
        }
    }
    return 0;
    if (ret > 0) return 0;
    else {
        printk("[kill] kill failed, no process found\n");
        return -ESRCH;
    }
}


int32_t do_tkill(int tid, int sig){
    do_kill(tid, sig);
}
int32_t do_tgkill(int tgid, int tid, int sig){
    do_kill(tid, sig);
}