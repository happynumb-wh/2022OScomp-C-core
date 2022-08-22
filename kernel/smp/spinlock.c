#include <atomic.h>
#include <os/spinlock.h>

// acquire the spin lock
void acquire(spinlock_t * spin){
    while(atomic_cmpxchg(UNLOCKED, LOCKED, (ptr_t)&spin->status))
    ;
    // the cpu handle the spinlock
    // spin->cpu = get_current_cpu_id();
}




// release the spin lock
// void release(spinlock_t * spin){
//     // set the lock status
//     spin->status = UNLOCKED;
// }

