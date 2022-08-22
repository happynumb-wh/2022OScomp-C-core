#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <os/spinlock.h>
#include <os/string.h>
#include <sys/special_ctx.h>
#include <fs/file.h>
#include <os/futex.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <csr.h>
#include <stdio.h>
/**
 * @brief init the kernel stack and the user stack
 *  the switch context and the save context 
 * @param kernel_stack kernel stack
 * @param user_stack No preprocessing user stack
 * @param entry_point the entry
 * @param pcb the pcb todo
 * @param argc the argc
 * @param argv the argv
 * @param envp the environment
 * no return 
 * 
 */
extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[])
{
    // trap pointer
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    kmemset(pt_regs, 0, sizeof(regs_context_t));
    

    /* command line parameter, retain for qemu test */
    // pt_regs->regs[10] = argc;
    // pt_regs->regs[11] = (uint64_t)argv;

    pcb->kernel_sp -= (sizeof(regs_context_t) + sizeof(switchto_context_t));
    
    // switch pointer
    switchto_context_t *switch_reg = (switchto_context_t *)( kernel_stack - 
                                                             sizeof(regs_context_t) -
                                                             sizeof(switchto_context_t)
                                                            );
    kmemset(switch_reg, 0, sizeof(switchto_context_t));     
    pcb->save_context = pt_regs;
    pcb->switch_context = switch_reg;

    // save all the things on the stack
    // pcb->user_sp = user_stack;

    // set context regs
    if(pcb->type == USER_PROCESS || pcb->type == USER_THREAD){
        pt_regs->regs[2] = (reg_t) pcb->user_sp;         //SP  
        pt_regs->regs[3] = (reg_t)__global_pointer$;  //GP
        pt_regs->regs[4] = (reg_t) pcb;               //TP
        // use sscratch to placed tp
        pt_regs->sscratch = (reg_t) pcb;
        pt_regs->sepc = entry_point;                  //sepc
        #ifndef k210
            pt_regs->sstatus = SR_SUM | SR_FS;        //sstatus
        #else
            pt_regs->sstatus = SR_FS;                 //sstatus
        #endif
    }

    // set switch regs
    switch_reg->regs[0] = (pcb->type == USER_PROCESS || pcb->type == USER_THREAD) ?
                                             (reg_t)&ret_from_exception : entry_point;                                                       //ra

    switch_reg->regs[1] = pcb->kernel_sp; 

    pcb->pge_num = 0;


}

/**
 * @brief init the clone process pcb
 * @param pcb the todo pcb
 * @return no return
 */
void init_clone_stack(pcb_t *pcb, void * tls)
{   
    pcb_t * current_running = get_current_running();

    /* trap pointer */
    pcb->save_context = \
        (regs_context_t *)(pcb->kernel_stack_top - sizeof(regs_context_t));
    /* switch regs */
    pcb->switch_context = (switchto_context_t *)( pcb->kernel_stack_top - 
                                                    sizeof(regs_context_t) -
                                                    sizeof(switchto_context_t)
                                                );
    /* copy all kernel stack */
    share_pgtable(pcb->kernel_stack_base, current_running->kernel_stack_base);
    pcb->kernel_sp = (uintptr_t) (pcb->switch_context);
    
    // some change trap reg
    pcb->switch_context->regs[0] = (reg_t)&ret_from_exception ;
    pcb->switch_context->regs[1] = pcb->kernel_sp             ;

    pcb->save_context->regs[2]   = pcb->user_stack_top == USER_STACK_ADDR ?  current_running->save_context->regs[2]\ 
                                        : pcb->user_stack_top;
    // the tp may not be the pcb
    if (pcb->flag & CLONE_SETTLS)
        pcb->save_context->regs[4] = (reg_t)tls               ;
    pcb->save_context->regs[10]  = 0                          ; // child return zero
    pcb->save_context->sscratch  = (reg_t)(pcb)               ; // sscratch save the pcb address

    init_list(&pcb->wait_list);
}

/**
 * @brief load process into memory
 * @name the process name
 */
uintptr_t load_process_memory(const char * path, pcb_t * initpcb)
{
    // #ifdef k210
    uint64_t length = 0;     
        #ifdef FAST
            ptr_t entry_point = fast_load_elf(path, initpcb->pgdir, &length, initpcb);
        #else        
            int fd;
            if((fd = fat32_openat(AT_FDCWD, path, O_RDONLY, NULL)) == -1) {
                prints("failed to open %s\n", path);
                return -ENOENT; 
            }        
            ptr_t entry_point = fat32_load_elf(         fd, 
                                            initpcb->pgdir, 
                                                &length,
                                                initpcb,
                                        alloc_page_helper
                                                    );
        #endif
    if(entry_point == 0)
        return -ENOEXEC;

    return entry_point;
}

/**
 * @brief init the pcb member
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 * no return
 */
void init_pcb_member(pcb_t * initpcb,  task_type_t type, spawn_mode_t mode)
{
    initpcb->pre_time = get_ticks();
    // id lock
    initpcb->pid = process_id++;

    initpcb->used = 1;
    initpcb->status = TASK_READY;
    initpcb->type = type;
    initpcb->mode = mode;
    initpcb->flag = 0;
    initpcb->parent.parent = get_current_running();
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    initpcb->execve = 0;
    clear_pcb_time(initpcb);
    initpcb->fd_limit.rlim_cur = MAX_FD_NUM;
    initpcb->fd_limit.rlim_max = MAX_FD_NUM;
    mylimit.rlim_cur = MAX_FD_NUM;
    mylimit.rlim_max = MAX_FD_NUM;
    init_list(&initpcb->wait_list);
    
    // fd table
    if (status_ctx)
        init_extend_fd(initpcb->ctx_pipe_array);
    else 
    {
        initpcb->pfd = alloc_fd_table();
        init_fd(initpcb->pfd);        
    }

    // set signal
    initpcb->sigactions = alloc_sig_table();
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;    
    //itimer
    initpcb->itimer.add_tick = 0;

    // init excellent load
    #ifdef FAST
        if (!kstrcmp(initpcb->name, "shell"))
            return;
        int exe_load_id = find_name_elf(initpcb->name);
        if (exe_load_id == -1) 
            printk("[Error] init execve pcb not find the %s\n", initpcb->name);
        else initpcb->exe_load = &pre_elf[exe_load_id];
    #endif
}

/**
 * @brief init the clone pcb
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 */
void init_clone_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode)
{
    pcb_t *current_running = get_current_running();

    initpcb->pre_time = get_ticks();
    // id lock
    initpcb->pid = process_id;
    initpcb->parent.parent = current_running;
    initpcb->ppid = current_running->pid;
    initpcb->tid = process_id++;
    initpcb->execve = 0;

    initpcb->used = 1;
    initpcb->status = TASK_READY;
    initpcb->type = type;
    initpcb->mode = mode;
    // initpcb->flag = 0;
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    clear_pcb_time(initpcb);
    init_list(&initpcb->wait_list);
    kmemcpy(&initpcb->fd_limit,&current_running->fd_limit,sizeof(struct rlimit));
    //itimer
    initpcb->itimer.add_tick = 0;
    // clone fd init
    if (initpcb->flag & CLONE_FILES) {
        initpcb->pfd = current_running->pfd;
        pfd_table_t* pt =  list_entry(current_running->pfd, pfd_table_t, pfd);
        pt->num++;
    }else{
        if (status_ctx)
        {
            copy_extend_fd(initpcb->ctx_pipe_array, current_running->ctx_pipe_array);
        } else 
        {
            initpcb->pfd = alloc_fd_table();
            // init_fd(initpcb->pfd);
            for (int i = 0; i < MAX_FD_NUM; i++)
            {
                /* code */
                copy_fd(&initpcb->pfd[i], &current_running->pfd[i]);            
            }            
        }
    }    

    // copy elf
    kmemcpy(&initpcb->elf, &current_running->elf, sizeof(ELF_info_t));
    initpcb->edata = current_running->edata;
    // set signal
    if (initpcb->flag & CLONE_SIGHAND) {
        initpcb->sigactions = current_running->sigactions;
        sigaction_table_t* st =  list_entry(current_running->sigactions, sigaction_table_t, sigactions);
        st->num++;
    }else{
        initpcb->sigactions = alloc_sig_table();
        kmemcpy(initpcb->sigactions,current_running->sigactions,NUM_SIG * sizeof(sigaction_t));
    } 
    //initpcb->sigactions = alloc_sig_table();
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;   
    // init excellent load
    #ifdef FAST
        initpcb->exe_load = current_running->exe_load;
    #endif
    
}

/**
 * @brief init the execve pcb
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 */
void init_execve_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode)
{
    pcb_t *current_running = get_current_running(); 

    initpcb->status = TASK_READY;
    initpcb->type = type;
    if (initpcb->mode == AUTO_CLEANUP_ON_EXIT)
        initpcb->mode = mode;
    /* stack message */
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    clear_pcb_time(initpcb);       
    // set signal
    //kmemset(initpcb->sigactions, 0, sizeof(sigaction_t) * NUM_SIG);
    initpcb->execve = 1;
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;   
    // set signal lock
    // init fd
    //init_fd(initpcb->pfd);
    // init excellent load
    #ifdef FAST
        int exe_load_id = find_name_elf(initpcb->name);
        if (exe_load_id == -1) 
            printk("[Error] init execve pcb not find the %s\n", initpcb->name);
        else initpcb->exe_load = &pre_elf[exe_load_id];
    #endif
}

/**
 * @brief handle pipe and redirect in exec
 * @param initpcb the init pcb
 * @param argc the argc
 * @param argv the argv
 * @return return the true argc num
 */
int handle_exec_pipe_redirect(pcb_t *initpcb, pid_t *pipe_pid, int argc, char* argv[])
{
    pcb_t * current_running = get_current_running();
    int mid_argc;
    for (mid_argc = 0; mid_argc < argc && kstrcmp(argv[mid_argc],"|"); mid_argc++);
    int redirect1 = !kstrcmp(argv[mid_argc - 2], ">");
    int redirect2 = !kstrcmp(argv[mid_argc - 2], ">>");
    int true_argc = (redirect1 | redirect2) ? mid_argc - 2 : mid_argc;
    // argv[true_argc] = NULL;
    if (mid_argc != argc) {
        assert(argv[mid_argc+1][0] == '.' && argv[mid_argc+1][1] == '/');
        argv[mid_argc+1] += 2;
        // the pipe_pid
        *pipe_pid = do_exec(argv[mid_argc+1],argc-(mid_argc+1),&argv[mid_argc+1],AUTO_CLEANUP_ON_EXIT);
        pcb_t *targetpcb;
        // ============ could be deleted because of the get_free_pcb will get one free always
        int i;
        for (i=0;i<NUM_MAX_TASK;i++){
            targetpcb = &pcb[i];
            if (targetpcb->pid == *pipe_pid) break;
        }
        if (i==NUM_MAX_TASK) return -ENOMEM;
        // ===============
        // alloc one pipe
        uint32_t pipenum = alloc_one_pipe();
        if ((int)pipenum == -ENFILE) return -ENFILE;
        // init pipe
        targetpcb->pfd[0].dev = DEV_PIPE;
        targetpcb->pfd[0].pip_num = pipenum;
        targetpcb->pfd[0].is_pipe_read = 1;
        targetpcb->pfd[0].is_pipe_write = 0;        
        initpcb->pfd[1].dev = DEV_PIPE;
        initpcb->pfd[1].pip_num = pipenum;
        initpcb->pfd[1].is_pipe_read = 0;
        initpcb->pfd[1].is_pipe_write = 1;        
    }
    // TODO
    if (redirect1 | redirect2)
    {
        // the redirect file name
        char *redirect_file = argv[mid_argc - 1];
        int fd = 0;
        fat32_close(1); //close STDOUT
        if(redirect1 && ((fd = fat32_openat(AT_FDCWD,redirect_file,O_WRONLY,0)) != -ENOENT)){
            fat32_close(fd);
            fat32_unlink(AT_FDCWD,redirect_file,0);
        }
        fd = fat32_openat(AT_FDCWD,redirect_file,O_WRONLY | O_CREATE,0);
        copy_fd(&initpcb->pfd[1],&current_running->pfd[get_fd_index(fd,current_running)]);
        initpcb->pfd[1].fd_num = 1;
        fat32_close(fd);
        if (redirect2) { // lseek
            fd_t *nfd = &initpcb->pfd[1];
            // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
            //     kmemcpy(nfd, nfd->share_fd, sizeof(fd_t));
            // }   
            nfd->pos = nfd->length;
            // nfd->version++;
            // kmemcpy(nfd->share_fd, nfd, sizeof(fd_t));
        }
    }    
    return true_argc;
}


/**
 * @brief set the argc and argv in the user stack 
 * @param argc the num of argv
 * @param argv the argv array
 * @param init_pcb the pcb 
 */
void set_argc_argv(int argc, char * argv[], pcb_t *init_pcb){
    /* get the kernel addr */
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    uintptr_t new_argv = init_pcb->user_stack_top - 0xc0;
    // uintptr_t new_argv_pointer = init_pcb->user_stack_top - 0x100;
    uintptr_t kargv = kustack_top - 0xc0;//get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = kustack_top - 0x100;//get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    *((uintptr_t *)kargv_pointer) = argc;
    for (int j = 1; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kstrcpy((char *)kargv , argv[j]);
        new_argv = new_argv + kstrlen(argv[j]) + 1;
        kargv = kargv + kstrlen(argv[j]) + 1;
    }     
}

/**
 * @brief set the std argc argv for std riscv
 * @param argc the num of argv
 * @param argv the argv array
 * @param init_pcb the pcb 
 */
void set_argc_argv_std(int argc, char * argv[], pcb_t *init_pcb)
{
    /* get the kernel addr */
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    uintptr_t new_argv = init_pcb->user_stack_top - 0xc0;
    // uintptr_t new_argv_pointer = init_pcb->user_stack_top - 0x100;
    uintptr_t kargv = kustack_top - 0xc0;//get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = kustack_top - 0x100;//get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    for (int j = 0; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kstrcpy((char *)kargv , argv[j]);
        new_argv = new_argv + kstrlen(argv[j]) + 1;
        kargv = kargv + kstrlen(argv[j]) + 1;
    }            
}

/**
 * @brief copy the argv, envp, and aux on the user stack.
 * @param pcb the pcb structure 
 * @param user_stack init user stack
 * @param argc num of argc
 * @param argv argv
 * @param envp environment
 * @param filename the filename
 */
uintptr_t copy_thing_user_stack(pcb_t *init_pcb, ptr_t user_stack, int argc, \
            char *argv[], char *envp[], char *filename)
{
    // the kva of the user_top
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    // caculate the space needed
    int envp_num = (int)get_num_from_parm(envp);

    // printk("argc: %d, envp_num: %d\n", argc, envp_num);
    // for (int i = 0; i < argc; i++)
    // {
    //     /* code */
    //     printk("%s\n", argv[i]);
    // }
    // for (int i = 0; i < envp_num; i++)
    // {
    //     /* code */
    //     printk("%s\n", envp[i]);
    // }
    
    int tot_length = (argc + envp_num + 3) * sizeof(uintptr_t *) + sizeof(aux_elem_t) * (AUX_CNT + 1) + SIZE_RESTORE;
    
    int point_length = tot_length;
    // get the space of the argv
    for (int i = 0; i < argc; i++)
    {
        tot_length += (kstrlen(argv[i]) + 1);
    }
    // get the space of the envp
    for (int i = 0; i < envp_num; i++)
    {
        tot_length += (kstrlen(envp[i]) + 1);
    }
    
    tot_length += (kstrlen(filename) + 1);
    // for random
    tot_length += (0x10);
    tot_length = ROUND(tot_length, 0x100);

    // printk("tot_length: 0x%lx\n", tot_length);

    uintptr_t kargv_pointer = kustack_top - tot_length;

    intptr_t kargv = kargv_pointer + point_length;
    
    // printk("point length: 0x%lx, kargv_pointer: 0x%lx, kargv: 0x%lx\n",point_length, kargv_pointer, kargv);
    /* 1. save argc */
    *((uintptr_t *)kargv_pointer) = argc; kargv_pointer += sizeof(uintptr_t*);
    /* 2. save argv */
    uintptr_t new_argv = init_pcb->user_stack_top - tot_length + point_length;
    for (int j = 0; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kstrcpy((char *)kargv , argv[j]);
        new_argv = new_argv + kstrlen(argv[j]) + 1;
        kargv = kargv + kstrlen(argv[j]) + 1;
    }
    // kargv_pointer += sizeof(uintptr_t*) * argc;
    *((uintptr_t *)kargv_pointer + argc) = NULL;

    kargv_pointer += (argc + 1) * sizeof(uintptr_t *);

    /* 3. save envp */
    for (int j = 0; j < envp_num; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kstrcpy((char *)kargv , envp[j]);
        new_argv = new_argv + kstrlen(envp[j]) + 1;
        kargv = kargv + kstrlen(envp[j]) + 1;
    }
    *((uintptr_t *)kargv_pointer + envp_num) = NULL;

    kargv_pointer += (envp_num + 1) * sizeof(uintptr_t *);

    /* 4. save the file name */
    kstrcpy((char *)kargv , filename);

    uintptr_t file_pointer = new_argv;
    new_argv = new_argv + kstrlen(filename) + 1;
    kargv = kargv + kstrlen(filename) + 1;

    /* 5. set aux */
    // aux_elem_t aux_vec[AUX_CNT];
    set_aux_vec((aux_elem_t *)kargv_pointer, &init_pcb->elf, file_pointer, new_argv);

    // printk("aux_vec: %lx \n", kargv_pointer);
    /* 6. copy aux on the user_stack */
    // kmemcpy(kargv_pointer, aux_vec, sizeof(aux_elem_t) * (AUX_CNT + 1));
    *((uintptr_t *)kargv_pointer + AUX_CNT + 1) = NULL;

    /* 7. copy the restore on the user stack */
    kmemcpy(kustack_top - SIZE_RESTORE, __restore, SIZE_RESTORE);
    /* 8. return user_stack */
    return init_pcb->user_stack_top - tot_length;
    
}

/**
 * @brief get argc num from argv
 * @param argv the argv array
 * @return return the argc num
 */
uint64_t get_num_from_parm(char *parm[])
{
    uint64_t num = 0;
    if (parm)
        while (parm[num]) num++;
    return num;
}
/**
 * @brief copy on write for clone 
 * 
 */
void copy_on_write(PTE src_pgdir, PTE dst_pgdir)
{
    PTE *kernelBase = (PTE *)pa2kva(PGDIR_PA);    
    PTE *src_base = (PTE *)src_pgdir;
    PTE *dst_base = (PTE *)dst_pgdir;
    int copy_num = 0;
    for (int vpn2 = 0; vpn2 < PTE_NUM; vpn2++)
    {
        /* kernel space or null*/
        if (kernelBase[vpn2] || !src_base[vpn2]) 
            continue;
        PTE *second_page_src = (PTE *)pa2kva((get_pfn(src_base[vpn2]) << NORMAL_PAGE_SHIFT));
        PTE *second_page_dst = (PTE *)check_page_set_flag(dst_base , vpn2, _PAGE_PRESENT);
        /* we will let the user be three levels page */ 
        for (int vpn1 = 0; vpn1 < PTE_NUM; vpn1++)
        {
            if (!second_page_src[vpn1]) 
            {
                // printk(" [Copy Error], second pgdir is null ?\n");
                continue;
            }
               
            PTE *third_page_src = (PTE *)pa2kva((get_pfn(second_page_src[vpn1]) << NORMAL_PAGE_SHIFT));
            PTE *third_page_dst = (PTE *)check_page_set_flag(second_page_dst , vpn1, _PAGE_PRESENT);
            for (int vpn0 = 0; vpn0 < PTE_NUM; vpn0++)
            {
                if (!third_page_src[vpn0] || third_page_dst[vpn0]) 
                {
                    // printk(" [Copy Error], third pgdir is null ?\n");
                    continue;                    
                }
                // ================= debug ====================
                // uint64_t vaddr = ((uint64_t)vpn0 << 12) | 
                //                  ((uint64_t)vpn1 << 21) |
                //                  ((uint64_t)vpn2 << 30);     
                // printk("fork vaddr: %lx\n", vaddr); 
                // ================= debug ====================                           
                /* page base */
                uint64_t share_base = pa2kva((get_pfn(third_page_src[vpn0]) << NORMAL_PAGE_SHIFT));
                /* close the W */
                uint64_t pte_flags = get_attribute(third_page_src[vpn0]) & (~_PAGE_WRITE);
                set_attribute(&third_page_src[vpn0], pte_flags);
                /* child */
                third_page_dst[vpn0] = third_page_src[vpn0];

                /* one more share the page */
                share_add(share_base);     
            }
        }        
    } 
}

/**
 * @brief handle the eixt pcb
 * @param exitPCB the exit pcb
 * @return no return
 * 
 */
void handle_exit_pcb(pcb_t * exitPcb)
{
    release_wait_deal_son(exitPcb);

    #ifdef FAST
        int recycle_page_num;
        if (!(exitPcb->flag & CLONE_VM)) 
            recycle_page_num = recycle_page_part(exitPcb->pgdir, exitPcb->exe_load, exitPcb->edata);
    #else
        int recycle_page_num;
        if (!(exitPcb->flag & CLONE_VM)) 
            recycle_page_num =recycle_page_default(exitPcb->pgdir);
    #endif
    recycle_pcb_default(exitPcb);
    freePage(exitPcb->kernel_stack_base);    
}


/**
 * @brief release all son and find new father
 * @param todoPcb the pcb be dealed
 * @return no return;
 */
void release_wait_deal_son(pcb_t * exitPcb)
{
    pcb_t *wait_entry = NULL, *wait_q;
    list_for_each_entry_safe(wait_entry, wait_q, &exitPcb->wait_list, list)
    {
        if (wait_entry->status == TASK_BLOCKED) 
        {
            do_unblock(&wait_entry->list);
        }
    }
    /* me will be the son's father */
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].parent.parent == exitPcb && pcb[i].used) 
        {
        if (pcb[i].status == TASK_ZOMBIE)
        {
            handle_exit_pcb(&pcb[i]);    
            return;
        }
            // find father
            pcb[i].parent.parent = exitPcb->parent.parent;
        }
    }                              
}

/**
 * @brief recycle the PCB
 * @return Succeed return 1;
 * Failed return -1;
 */
int recycle_pcb_default(pcb_t * recyclePCB){
    //debug
    int count = 0;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if(pcb[i].used == 0)
            count++;
    }
    // printk("[recyclepcb] pid: %d, free pcb count: %d\n", recyclePCB->pid, count);
    // free the pcb
    free_pcb(recyclePCB);

    /* do */
    recyclePCB->pge_num = 0;

    // free fd
    if (!status_ctx)
        free_fd_table(recyclePCB->pfd);
    // free sig
    free_sig_table(recyclePCB->sigactions);
    // itimer
    if(recyclePCB->itimer.add_tick != 0){
        recyclePCB->itimer.add_tick = 0;
        list_del(&recyclePCB->itimer.list);
        kmemset(&recyclePCB->itimer, 0, sizeof(timer_t));
    }
    
    for (int i = 0; i < MAX_FUTEX_NUM; i++)
        if (futex_node_used[i] == recyclePCB->pid) {
            list_del(&(futex_node[i].list));
            futex_node_used[i] = 0;
        }
    // freePage(recyclePCB->kernel_stack_top - NORMAL_PAGE_SIZE);
    return 0;
}

/**
 * @brief find one free pcb for the handler 
 * @return Success return the pcb sub failed return -1
 * 
 */
pcb_t* get_free_pcb(){
    list_head *freePcbList = &available_queue.free_source_list;
    pcb_t * current_running = get_current_running();
try:    if (!list_empty(freePcbList)){
        list_node_t* new_pcb = freePcbList->next;
        pcb_t *new = list_entry(new_pcb, pcb_t, list);
        /* one use */
        list_del(new_pcb);
        // list_add_tail(new_pcb, &used_queue);

        // printk("alloc mem kva 0x%lx\n", new->kva_begin);
        return new;        
    }else{
        // memCurr += PAGE_SIZE;    
        printk("Failed to allocPcb, I am pid: %d\n", current_running->pid);
        do_block(&current_running->list, &available_queue.wait_list);
        goto try;
    }
    return -1;
}

/**
 * @brief add the pcb to the available_queue
 * @param RecyclePcb the recycle pcb
 * @return no return
 */
void free_pcb(pcb_t *RecyclePcb)
{
    RecyclePcb->used = 0;
    RecyclePcb->status = TASK_EXITED;
    RecyclePcb->tid = 0;
    RecyclePcb->clear_ctid = 0;
    // get the freePcbList
    list_head *freePcbList = &available_queue.free_source_list;
    if (list_empty(&RecyclePcb->list))    
        list_del(&RecyclePcb->list);
    list_add(&RecyclePcb->list, freePcbList);
    // we only free one
    if (!list_empty(&available_queue.wait_list))
        do_unblock(available_queue.wait_list.next);
    return;
}