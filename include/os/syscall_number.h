/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
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

#ifndef OS_SYSCALL_NUMBER_H_
#define OS_SYSCALL_NUMBER_H_
/*************for shell*************/
#define IGNORE 0
#define NUM_SYSCALLS 512

/* define */  
#define SYS_getcwd              (17) 
#define SYS_dup                 (23)  
#define SYS_dup3                (24) 
#define SYS_fcntl               (25)
#define SYS_ioctl               (29)
#define SYS_mkdirat             (34)  
#define SYS_unlinkat            (35)   
#define SYS_linkat              (37)   
#define SYS_umount2             (39)   
#define SYS_mount               (40)   
#define SYS_statfs              (43)
#define SYS_fstatfs             (44)
#define SYS_faccessat           (48)
#define SYS_chdir               (49)   
#define SYS_openat              (56)  
#define SYS_close               (57)   
#define SYS_pipe2               (59)     
#define SYS_getdents64          (61)
#define SYS_lseek               (62)
#define SYS_read                (63)  
#define SYS_write               (64)   
#define SYS_readv               (65)
#define SYS_writev              (66)
#define SYS_pread               (67)   
#define SYS_pwrite              (68)
#define SYS_sendfile            (71)
#define SYS_pselect6            (72)
#define SYS_ppoll               (73)
#define SYS_readlinkat          (78)
#define SYS_fstatat             (79)
#define SYS_fstat               (80) 
#define SYS_fsync               (82)
#define SYS_utimensat           (88)
#define SYS_exit                (93)  
#define SYS_exit_group          (94)
#define SYS_set_tid_address     (96)
#define SYS_futex               (98)
#define SYS_set_robust_list     (99)
#define SYS_get_robust_list     (100)
#define SYS_nanosleep           (101) 
#define SYS_setitimer           (103)
#define SYS_clock_gettime       (113)
#define SYS_syslog              (116)
#define SYS_sched_setscheduler  (119)
#define SYS_sched_getaffinity   (123)
#define SYS_sched_yield         (124) 
#define SYS_kill                (129)
#define SYS_tkill               (130)
#define SYS_tgkill              (131)
#define SYS_rt_sigaction        (134)
#define SYS_rt_sigprocmask      (135)
#define SYS_rt_sigtimedwait     (137)
#define SYS_rt_sigreturn        (139)
#define SYS_times               (153) 
#define SYS_setsid              (157)
#define SYS_uname               (160) 
#define SYS_getrlimit           (163)
#define SYS_setrlimit           (164)
#define SYS_getrusage           (165)
#define SYS_vm86                (166)
#define SYS_gettimeofday        (169) 
#define SYS_getpid              (172)
#define SYS_getppid             (173) 
#define SYS_getuid              (174)
#define SYS_geteuid             (175)
#define SYS_getgid              (176)
#define SYS_getegid             (177)
#define SYS_gettid              (178)
#define SYS_sysinfo             (179)
#define SYS_socket              (198)
#define SYS_bind                (200)
#define SYS_listen              (201)
#define SYS_accept              (202)
#define SYS_connect             (203)
#define SYS_getsockname         (204)
#define SYS_sendto              (206)
#define SYS_recvfrom            (207)
#define SYS_setsockopt          (208)
#define SYS_brk                 (214) 
#define SYS_munmap              (215) 
#define SYS_mremap              (216)
#define SYS_clone               (220)  
#define SYS_execve              (221) 
#define SYS_mmap                (222)  
#define SYS_mprotect            (226)
#define SYS_msync               (227)
#define SYS_madvise             (233)
#define SYS_exec                (243)
#define SYS_screen_write        (244)
#define SYS_screen_reflush      (245)
#define SYS_screen_clear        (246)
#define SYS_get_timer           (247)
#define SYS_get_tick            (248)
#define SYS_shm_pageget         (249)
#define SYS_shm_pagedt          (250)
#define SYS_disk_read           (251)
#define SYS_disk_write          (252)
#define SYS_preload             (253)
#define SYS_extend              (254)

#define SYS_wait4               (260) 
#define SYS_prlimit             (261)
#define SYS_renameat2           (276)
#define SYS_membarrier          (283)
#endif
// TODO:


