### #define SYS_fcntl               (25)

```c++
int fcntl(int fd, int cmd);
int fcntl(int fd, int cmd, long arg);         
int fcntl(int fd, int cmd, struct flock *lock);
```

fcntl()针对(文件)描述符提供控制。参数fd是被参数cmd操作(如下面的描述)的描述符；针对cmd的值,fcntl能够接受第三个参数（arg）。

fcntl函数有5种功能：

1.复制一个现有的描述符（cmd=F_DUPFD）.

2.获得／设置文件描述符标记(cmd=F_GETFD或F_SETFD).

3.获得／设置文件状态标记(cmd=F_GETFL或F_SETFL).

4.获得／设置异步I/O所有权(cmd=F_GETOWN或F_SETOWN).

5.获得／设置记录锁(cmd=F_GETLK,F_SETLK或F_SETLKW).

fcntl的返回值与命令有关。如果出错，所有命令都返回－1，如果成功则返回某个其他值。下列三个命令有特定返回值：F_DUPFD,F_GETFD,F_GETFL以及F_GETOWN。第一个返回新的文件描述符，第二个返回相应标志，最后一个返回一个正的进程ID或负的进程组ID。

具体参考下面链接中的具体定义予以实现。

（实际的测试代码里应该只有GETLK,SETLK,GETFD,GETFL；其他的（比如异步I/O）疑似不用实现）

[FCNTL - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/fcntl64.2.html)

```c
// ./kernel/fat32/fat32.c
int32_t fat32_fcntl(fd_num_t fd, int32_t cmd, int32_t arg);
```



### #define SYS_ioctl               (29)

**待进一步研究！**

控制I/O设备；int ioctl(int fd, ind cmd, …)； 

（测试源码内没有直接调用ioctl，间接使用？所以不知道具体怎么用到的，由于是聚合函数也不知道怎么实现，也许可以从编译链/测试源码搜集更多资料，或直接返回0试试？）

[IOCTL - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/ioctl.2.html)

```c
// ./kernel/io/io.c
int64_t do_ioctl(fd_num_t fd, uint64_t request, const char *argp[]);(return 0)
```

### #define SYS_statfs              (43) / #define SYS_fstatfs             (44)

查询文件系统相关的信息。return 0 / -1。

```c
    struct statfs {
       long f_type; /* 文件系统类型 */
       long f_bsize; /* 经过优化的传输块大小 */
       long f_blocks; /* 文件系统数据块总数 */
       long f_bfree; /* 可用块数 */
       long f_bavail; /* 非超级用户可获取的块数 */
       long f_files; /* 文件结点总数 */
       long f_ffree; /* 可用文件结点数 */
       fsid_t f_fsid; /* 文件系统标识 */
       long f_namelen; /* 文件名的最大长度 */
    };
```

[STATFS - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/fstatfs.2.html)

```c
// 学长未实现
int statfs(const char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);
```

### #define SYS_lseek               (62)

改文件指针，由于很熟悉所以不多说。应该已经实现了，只要加个系统调用的壳子就行。

[LSEEK - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/lseek.2.html)

```c
// 我们实现过了
off_t lseek(int fildes, off_t offset, int whence);
```

### #define SYS_readv               (65) / #define SYS_writev              (66)

readv和writev函数用于在一次函数调用中读、写多个非连续缓冲区。若成功则返回已读、写的字节数，若出错则返回-1。（参考链接理解）

readv()代表分散读， 即将数据从文件描述符读到分散的内存块中；writev()代表集中写，即将多块分散的内存一并写入文件描述符中。

```c
struct iovec {
    void *iov_base;   /* Starting address */
    size_t iov_len;   /* Number of bytes */
};
```

[READV - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/pwritev2.2.html)

[高级I/O之readv和writev函数 - ITtecman - 博客园 (cnblogs.com)](https://www.cnblogs.com/nufangrensheng/p/3559304.html)

```c
// ./kernel/fat32/read.c
int64 fat32_readv(fd_num_t fd, struct iovec *iov, int iovcnt);
// ./kernel/fat32/write.c
int64 fat32_writev(fd_num_t fd, struct iovec *iov, int iovcnt);
```

### #define SYS_ppoll               (73)

**待进一步研究！**

等待某个文件描述符上的某些事件发生。

（什么事件？怎么检测？我们应该不支持网络吧？）

[POLL - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/ppoll.2.html)

```c
// ./kernel/fat32/poll.c
int32_t do_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask);
```

### #define SYS_fstatat             (79)

访问文件信息（功能与fstat一致，获取目标文件的方式不同）。

[STAT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/fstatat.2.html)

[fstatat()函数 Unix/Linux - Unix/Linux系统调用 (yiibai.com)](https://www.yiibai.com/unix_system_calls/fstatat.html)

```c
// ./kernel/fat32/stat.c
int fat32_fstatat(fd_num_t dirfd, const char *pathname, struct stat *statbuf, int32 flags);
```

### #define SYS_utimensat           (88)

以纳秒级的精度改变文件的时间戳。times[0]修改最后一次访问的时间，times[1]修改最后修改的时间。

[UTIMENSAT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/utimensat.2.html)

```c
// ./kernel/fat32/file.c
int32_t do_utimensat(fd_num_t dirfd, const char *pathname, const struct timespec times[2], int32_t flags);
```

### #define SYS_exit_group          (94)

相当于exit（2），它不仅终止调用线程，还终止调用进程线程组中的所有线程。

[EXIT_GROUP - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/exit_group.2.html)

```c
// ./kernel/sched/sched.c
void do_exit_group(int32_t exit_status); //直接do_exit套皮
```

### #define SYS_set_tid_address     (96)

将指针设置为线程ID（将调用线程的clear_child_tid值设置为tidptr）。(啥意思？详见链接)

[SET_TID_ADDRESS - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/set_tid_address.2.html)

```c
// ./kernel/sched/id.c
pid_t do_set_tid_address(int *tidptr); // only return tid
```

### #define SYS_futex               (98)

快速的用户空间锁定

futex()系统调用提供了一种等待直到特定条件变为真的方法。它通常在共享内存同步的上下文中用作阻止结构。使用futex时，大多数同步操作在用户空间中执行。用户空间程序仅在程序必须阻塞更长的时间直到条件变为真时才使用futex()系统调用。其他futex()操作可用于唤醒等待特定条件的任何进程或线程。

（用户态锁？好像实现相当复杂）

[FUTEX - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/futex.2.html)

```c
// ./kernel/locking/futex.c; 学长虽然实现了futex的wakeup和wait，但是没有实现打包好的调用，需要费一番功夫
int futex(int *uaddr, int futex_op, int val,
          	const struct timespec *timeout,   /* or: uint32_t val2 */
         	 int *uaddr2, int val3);
```

### #define SYS_set_robust_list     (99) / #define SYS_get_robust_list     (100)

这些系统调用处理每个线程的健壮的futex列表。这些列表是在用户空间中管理的：内核仅知道列表头部的位置。线程可以使用set_robust_list()将其健壮的futex列表的位置通知内核。可以使用get_robust_list()获得线程健壮的futex列表的地址。

（健壮的futex列表的目的是：确保如果一个线程在终止或调用execve(2)之前意外未能解锁一个futex，则将通知另一个等待该futex的线程该futex的前所有者已死亡）

[GET_ROBUST_LIST - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/set_robust_list.2.html)

```c
// 学长未实现
long get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
long set_robust_list(struct robust_list_head *head, size_t len);
```

### #define SYS_clock_gettime       (113)

检索clockid指定时钟的时间。

（为什么没有添加配套的函数（下同，get总有set）？clockid怎么用？估计还得看看测试源码）

[CLOCK_GETRES - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/clock_gettime.2.html)

```c
// ./kernel/time/system_time.c
int32_t do_clock_gettime(uint64_t clock_id, struct timespec *tp);//就是gettimeofday
```

### #define SYS_sched_setscheduler  (119)

设置调度策略/参数（详见链接）

```c
struct sched_param {
    int sched_priority;
};
```

（我们只有一个调度策略吧？这个东西对测试会有影响吗？？）

[SCHED_SETSCHEDULER - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/sched_setscheduler.2.html)

```c
// 学长疑似没有实现，但是lmbench里有？直接装不存在？
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
```

### #define SYS_sched_getaffinity   (123)

获取线程的CPU亲和力掩码；将ID为pid的线程的亲和力掩码写入mask所指向的cpu_set_t结构中。 cpusetsize参数指定掩码的大小(以字节为单位)。如果pid为零，则返回调用线程的掩码。（就是get一个预设的mask？）

（这个东西对测试会有影响吗？return全1可以？）

[SCHED_SETAFFINITY - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/sched_getaffinity.2.html)

```c
// 学长未实现，建议直接return全1的掩码
int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
```

### #define SYS_kill                (129)

kill()系统调用可用于将任何信号发送到任何进程组或进程。（具体如何实现？）

[KILL - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/kill.2.html)

```c
// ./kernel/sched/sched.c
int32_t do_kill(pid_t pid, int32_t sig);
```

### #define SYS_tkill               (130)

tkill()将信号sig发送到具有线程ID tid的线程。（同上）

[TKILL - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/tkill.2.html)

```c
// ./kernel/sched/sched.c; 和之前的exit_group一样，直接do_kill套皮
// 线程组疑似都完全没有实现？
int do_tkill(int tid, int sig);
```

### #define SYS_rt_sigaction        (134)

用于更改进程在收到特定信号后采取的操作。

（最初的Linux系统调用名为sigaction()。但是，在Linux 2.2中添加了实时信号后，该系统调用支持的固定大小的32位sigset_t类型不再适合此目的。因此，添加了新的系统调用rt_sigaction()以支持扩展的sigset_t类型。新的系统调用采用第四个参数size_t sigsetsize，它指定act.sa_mask和oldact.sa_mask中信号集的大小(以字节为单位)。当前需要此参数具有sizeof(sigset_t)值(或错误EINVAL结果)。 glibc sigaction()包装函数对我们隐藏了这些细节，在内核提供时透明地调用rt_sigaction()。）（rt前缀的作用，下同）

[SIGACTION - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/rt_sigaction.2.html)

```c
// ./kernel/sched/signal.c
int32_t do_rt_sigaction(int32_t signum, struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
// 结合对应的头文件理解
```

### #define SYS_rt_sigprocmask      (135)

用于获取和/或更改调用线程的信号掩码。信号掩码是当前被呼叫者阻止传递的信号集。

[SIGPROCMASK - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/sigprocmask.2.html)

```c
// ./kernel/sched/signal.c
int32_t do_rt_sigprocmask(int32_t how, const sigset_t *restrict set, sigset_t *restrict oldset, size_t sigsetsize)
```

### #define SYS_rt_sigtimedwait     (137)

暂停调用线程的执行，直到set中的信号之一挂起(如果set中的信号之一已经为调用线程挂起将立即返回。)它具有一个附加参数timeout，该参数指定线程挂起等待信号的间隔。返回信号号或-1。

[SIGWAITINFO - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/rt_sigtimedwait.2.html)

```c
// 需要自己实现；藏得真深？
int sigtimedwait(const sigset_t *set, soirnfo_t *info,
                 const struct timespec *timeout);
```

### #define SYS_rt_sigreturn        (139)

从信号处理程序和清除堆栈帧返回。（啥意思？见链接）

[SIGRETURN - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/sigreturn.2.html)

```c
// ./kernel/sched/signal.c
void do_rt_sigreturn();
```

### #define SYS_setsid              (157)

创建会话并设置进程组ID

[SETSID - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/setsid.2.html)

```c
//学长未实现，考虑到进程组整个没用，不知道可不可以直接套壳 return 0？
pid_t setsid(void);
```

### #define SYS_getrlimit           (163) / #define SYS_setrlimit           (164)

getrlimit()和setrlimit()系统调用获取和设置资源限制。

```c
struct rlimit {
    rlim_t rlim_cur;  /* Soft limit */
    rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};
```

[GETRLIMIT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/getrlimit.2.html)

```c
// 学长未实现
// 为什么很多lmbench里有的系统调用学长都没实现？？很奇怪
int getrlimit(int *resource*，struct rlimit * rlim);
int setrlimit(int *resource*，const struct rlimit * rlim);
```

### #define SYS_geteuid             (175)

geteuid()返回调用过程的有效用户ID。

[GETUID - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/geteuid.2.html)

```c
// ./kernel/sched/id.c
uid_t geteuid(void);
// 学长的实现：直接return 0
```

### #define SYS_getegid             (177)

getegid()返回调用过程的有效组ID。

[GETGID - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/getegid.2.html)

```c
// ./kernel/sched/id.c
gid_t getegid(void);
// 学长的实现：直接return 0
```

### #define SYS_gettid              (178)

gettid()返回调用者的线程ID(TID)。

[GETTID - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/gettid.2.html)

```c
// ./kernel/sched/id.c
pid_t gettid(void);
// 学长的实现：直接return tid
```

### #define SYS_sysinfo             (179)

sysinfo()返回有关内存和交换使用情况以及平均负载的某些统计信息。

```c
struct sysinfo {
    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* Swap space still available */
    unsigned short procs;    /* Number of current processes */
    unsigned long totalhigh; /* Total high memory size */
    unsigned long freehigh;  /* Available high memory size */
    unsigned int mem_unit;   /* Memory unit size in bytes */
    char _f[20-2*sizeof(long)-sizeof(int)];
                             /* Padding to 64 bytes */
};
```

[SYSINFO - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/sysinfo.2.html)

```c
// ./kernel/system/system.c
int32_t do_sysinfo(struct sysinfo *info);
// 此处赋值很有参考价值
```

### #define SYS_socket              (198)

套接字-创建通信端点

意义不明的网络调用？（下同）

其实计网实验也用过作用直接看链接吧。

[SOCKET - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/socket.2.html)

```c
// 网络调用，直接没实现
int socket(int domain, int type, int protocol);
```

### #define SYS_bind                (200)

同上。

[BIND - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/bind.2.html)

```c
// 网络调用，直接没实现
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

### #define SYS_listen              (201)

同上。

[LISTEN - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/listen.2.html)

```c
// 网络调用，直接没实现
int listen(int sockfd, int backlog);
```

### #define SYS_getsockname         (204)

同上。（在addr指向的缓冲区中返回套接字sockfd绑定到的当前地址）

[GETSOCKNAME - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/getsockname.2.html)

```c
// 网络调用，直接没实现
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### #define SYS_setsockopt          (208)

同上。（操纵文件描述符sockfd所引用的套接字的选项。选项可能存在于多个协议级别。它们始终位于最上层的套接字级别。）

[GETSOCKOPT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/setsockopt.2.html)

```c
// 网络调用，直接没实现
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```

### #define SYS_mremap              (216)

mremap()扩展(或收缩)现有的内存映射，有可能同时移动它(由flags参数和可用的虚拟地址空间控制)。

[MREMAP - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/mremap.2.html)

```c
// 学长没实现
void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
```

### #define SYS_mprotect            (226)

mprotect()更改包含间隔[addr，addr + len-1]中地址范围任何部分的调用进程的内存页面的访问保护。 addr必须与页面边界对齐。

[MPROTECT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/mprotect.2.html)

```c
// ./kernel/mm/mman.c
int do_mprotect(void *addr, size_t len, int prot)
```

### #define SYS_madvise             (233)

madvise()系统调用用于向内核提供有关地址范围的建议或指导，这些地址范围始于地址addr且具有大小长度字节。在大多数情况下，此类建议的目的是提高系统或应用程序的性能。(？？？)

[MADVISE - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/madvise.2.html)

```c
// 学长没实现，我也不知道怎么实现，随便给个值？
int madvise(void* addr，size_t len，int notice);
```

### #define SYS_prlimit             (261)

获取/设置资源限制（？）

[GETRLIMIT - Linux手册页-之路教程 (onitroad.com)](https://www.onitroad.com/jc/linux/man-pages/linux/man2/prlimit.2.html)

```c
// ./kernel/system/resource.c
int do_prlimit(pid_t pid,   int resource,  const struct rlimit *new_limit, 
    struct rlimit *old_limit);
```

### 总结

总的来说，需要完全自己实现的syscall如下：

```
 SYS_statfs / SYS_fstatfs
 SYS_futex
 SYS_set_robust_list / SYS_get_robust_list
 SYS_sched_setscheduler / SYS_sched_getaffinity
 SYS_setsid
 SYS_getrlimit / SYS_setrlimit
 SYS_socket / SYS_bind / SYS_listen / SYS_getsockname / SYS_setsockopt
 SYS_mremap
 SYS_madvise
 SYS_rt_sigtimedwait
```

其他的可以通过移植和微改实现。

此外，还有一系列其他问题。
