#include <sys/syscall.h>
// #include <sys/shm.h>
#include <stdint.h>
#include <os.h>
// #include <time.h>
#include "time.h"
/*睡眠*/
#define NULL 0
int sys_sleep(unsigned long long time)
{

    TimeVal tv = {.sec = time, .usec = 0}; 
    if (invoke_syscall(SYS_nanosleep, &tv, &tv, IGNORE, IGNORE, IGNORE, IGNORE)) return tv.sec;
    return 0;
    // invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYS_screen_write, (uintptr_t)buff, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(){
    invoke_syscall(SYS_screen_clear, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYS_screen_reflush, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}


long sys_get_tick()
{
    return invoke_syscall(SYS_get_tick, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}
uint64_t sys_get_timer()
{
    return invoke_syscall(SYS_get_timer, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}
void sys_yield()
{
    invoke_syscall(SYS_sched_yield, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}


void sys_exit(void){
    invoke_syscall(SYS_exit,  IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    return invoke_syscall(SYS_exec, file_name, argc, argv, mode, IGNORE, IGNORE);
}

pid_t sys_load(const char *path, char* argv[], char *env[]) {
    return invoke_syscall(SYS_execve, path, argv, env, IGNORE, IGNORE, IGNORE);
}

void sys_test(const char *file_name) {
    sys_exec(file_name, NULL, NULL, AUTO_CLEANUP_ON_EXIT);
}

void* shmpageget(int key){
    return (void *)invoke_syscall(SYS_shm_pageget, key, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr){
    invoke_syscall(SYS_shm_pagedt, addr, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t getpid(){
    return invoke_syscall(SYS_getpid, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int open(const char *path, int flags)
{
    return invoke_syscall(SYS_openat, AT_FDCWD, path, flags, 0, IGNORE, IGNORE);
}
int32_t close(uint32_t fd){
    return invoke_syscall(SYS_close, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}
uint8_t sys_disk_read(uint8_t *data_buff, uint32_t sector, uint32_t count){
    return invoke_syscall(SYS_disk_read, data_buff, sector, count, IGNORE, IGNORE, IGNORE);
}
uint8_t sys_disk_write(uint8_t *data_buff, uint32_t sector, uint32_t count){
    return invoke_syscall(SYS_disk_write, data_buff, sector, count, IGNORE, IGNORE, IGNORE);
}

pid_t sys_wait(pid_t pid, uint16_t *status, uint32_t options)
{
    return invoke_syscall(SYS_wait4, pid, status, options, IGNORE, IGNORE, IGNORE);

}

int sys_pre_load(const char * file_name, int how)
{
    return invoke_syscall(SYS_preload, file_name, how, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint64_t extend(uint64_t num, int how, uint64_t threlod)
{
    return invoke_syscall(SYS_extend, num, how, threlod, IGNORE, IGNORE, IGNORE);
}