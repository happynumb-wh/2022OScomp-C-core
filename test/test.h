#ifndef USER_TEST_H
#define USER_TEST_H
#define NULL 0
#define MAX_ARGV_NUM 14
typedef struct final_test{
    int argc;
    char * argv[14];
} final_test_t;

static char *busybox_test[] = {
    "exec busybox clear",
    "exec busybox echo \"#### independent command test\"",
    "exec busybox ash -c exit",
    "exec busybox sh -c exit",
    "exec busybox basename /aaa/bbb",
    "exec busybox cal",
    "exec busybox date",
    "exec busybox df",
    "exec busybox dirname /aaa/bbb",
    "exec busybox dmesg",
    "exec busybox du",
    "exec busybox expr 1 + 1",
    "exec busybox false",
    "exec busybox true",
    "exec busybox which ls", 
    "exec busybox uname",
    "exec busybox uptime",
    "exec busybox ps",       
    "exec busybox pwd",
    "exec busybox free",     
    "exec busybox hwclock",  
    "exec busybox kill 10",
    "exec busybox ls",       
    "exec busybox sleep 1",
    "exec busybox echo \"#### file opration test\"",
    "exec busybox touch test.txt",
    "exec busybox echo \"hello world\" > test.txt",
    "exec busybox cat test.txt",
    "exec busybox cut -c 3 test.txt",
    "exec busybox od test.txt",
    "exec busybox head test.txt",
    "exec busybox tail test.txt",
    "exec busybox hexdump -C test.txt",
    "exec busybox md5sum test.txt",
    "exec busybox echo \"ccccccc\" >> test.txt",
    "exec busybox echo \"bbbbbbb\" >> test.txt",
    "exec busybox echo \"aaaaaaa\" >> test.txt",
    "exec busybox echo \"2222222\" >> test.txt",
    "exec busybox echo \"1111111\" >> test.txt",
    "exec busybox echo \"bbbbbbb\" >> test.txt",
    "exec busybox cat test.txt",
    "exec busybox sort test.txt | ./busybox uniq",
    "exec busybox stat test.txt",
    "exec busybox strings test.txt",
    "exec busybox wc test.txt",
    "exec busybox [ -f test.txt ]",
    "exec busybox more test.txt",
    "exec busybox rm test.txt",
    "exec busybox mkdir test_dir",
    "exec busybox mv test_dir test",
    "exec busybox rmdir test",
    "exec busybox grep hello busybox_cmd.txt",
    "exec busybox cp busybox_cmd.txt busybox_cmd.bak",
    "exec busybox rm busybox_cmd.bak",
    "exec busybox find -name \"busybox_cmd.txt\"",
    NULL   
};

static char * pre_lmbench_test[] = {
    "exec busybox mkdir -p /var/tmp",
    "exec busybox touch /var/tmp/lmbench",
    // "exec busybox cp hello /tmp",                                               
    NULL
};

static char * lmbench_test[] = {
    // "exec echo latency measurements",
    "exec lmbench_all lat_syscall -P 1 null",
    "exec lmbench_all lat_syscall -P 1 read",
    "exec lmbench_all lat_syscall -P 1 write",
    "exec lmbench_all lat_syscall -P 1 stat /var/tmp/lmbench",
    "exec lmbench_all lat_syscall -P 1 fstat /var/tmp/lmbench",
    "exec lmbench_all lat_syscall -P 1 open /var/tmp/lmbench",
    "exec lmbench_all lat_select -n 100 -P 1 file",
    "exec lmbench_all lat_sig -P 1 install",
    "exec lmbench_all lat_sig -P 1 catch",
    "exec lmbench_all lat_pipe -P 1",
    "exec lmbench_all lat_proc -P 1 fork",
    "exec lmbench_all lat_proc -P 1 exec",
    "exec lmbench_all lat_proc -P 1 shell",      
    "exec lmbench_all lmdd label=\"File /var/tmp/XXX write bandwidth:\" of=/var/tmp/XXX move=1m fsync=1 print=3",
    "exec lmbench_all lat_pagefault -P 1 /var/tmp/XXX",  //time too long
    "exec lmbench_all lat_mmap -P 1 512k /var/tmp/XXX",
    // "exec echo file system latency",
    "exec lmbench_all lat_fs /var/tmp",
    // "exec echo Bandwidth measurements",
    "exec lmbench_all bw_pipe -P 1",
    "exec lmbench_all bw_file_rd -P 1 512k io_only /var/tmp/XXX",
    "exec lmbench_all bw_file_rd -P 1 512k open2close /var/tmp/XXX",
    "exec lmbench_all bw_mmap_rd -P 1 512k mmap_only /var/tmp/XXX",
    "exec lmbench_all bw_mmap_rd -P 1 512k open2close /var/tmp/XXX",
    // "exec echo context switch overhead",
    "exec lmbench_all lat_ctx -P 1 -s 32 2 4 8 16 24 32",                  // not print info; 4 8 16 24 32 64 96
    "exec lmbench_all lat_ctx -P 1 -s 32 64",
    // "exec lmbench_all lat_ctx -P 1 -s 32 ",
    NULL
};

static char * lua_test[] = {
    "exec lua date.lua",
    "exec lua file_io.lua",
    "exec lua max_min.lua",
    "exec lua random.lua",
    "exec lua remove.lua",
    "exec lua round_num.lua",
    "exec lua sin30.lua",
    "exec lua sort.lua",
    "exec lua strings.lua",    
    NULL
};

#endif