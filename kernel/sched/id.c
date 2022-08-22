#include <os/list.h>
#include <screen.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <os/spinlock.h>
#include <stdio.h>
#include <assert.h>
#include <os/string.h>

pid_t do_getpid(){
    pcb_t *current_running = get_current_running();
    return current_running->pid;
}


pid_t do_getppid(){
    pcb_t *current_running = get_current_running();
    return current_running->ppid;
}

pid_t do_set_tid_address(int *tidptr)
{
    pcb_t *current_running = get_current_running();
    current_running->clear_ctid = tidptr;
    return current_running->tid;
}

pid_t do_setsid(void){
    return 0;
    //is this correct?
}

uid_t do_geteuid(void){
    return 0;
}

gid_t do_getegid(void){
    return 0;
}

pid_t do_gettid(void){
    pcb_t *current_running = get_current_running();
    return current_running->tid;
}
gid_t do_getgid(void){
    return 0;
}
uid_t do_getuid(void){
    return 0;
}