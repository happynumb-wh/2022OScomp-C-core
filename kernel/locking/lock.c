#include <os/lock.h>
#include <os/sched.h>
#include <os/mm.h>
#include <atomic.h>
#include <screen.h> 
#include <os/string.h>
// mutex_init(lock);
/* init one mutex lock */
void do_mutex_lock_init(mutex_lock_t *lock)
{
    init_list(&lock->block_queue);//初始化互斥锁队列
    lock->status = UNLOCKED;
    lock->pid = NULL;
}
/* acquire a mutex lock */
void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    pcb_t * current_running = get_current_running();

    while(lock->status == LOCKED){
        do_block(&current_running->list, &lock->block_queue);
    }

    lock->status = LOCKED;
    lock->pid = current_running->pid;
}

/* release one progress */
void do_mutex_lock_release(mutex_lock_t *lock)
{
    pcb_t * current_running = get_current_running();
    if(!list_empty(&lock->block_queue)){
        do_unblock(lock->block_queue.prev);//释放一个锁进程        
    }
    else{
        lock->status = UNLOCKED;
        lock->pid = 0;
    }
}
