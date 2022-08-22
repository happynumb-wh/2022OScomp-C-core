#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include "test.h"
#define COMMAND_NUM 1
#define True 1
#define False 0
#define SHELL_BEGIN 25
typedef void (*function)(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);
typedef void * parameter;

static pid_t shell_exec(final_test_t* command);

static struct{
    char *name;
    function func;
    int arg_num;
} command_table [] = {
        {"exec"   , &shell_exec   ,   1},
    };

static pid_t shell_exec(final_test_t* command){   
    // printf("exec process: %s\n", file_name);
    // sys_reflush();
    pid_t pid = sys_exec(command->argv[0], command->argc, command->argv, AUTO_CLEANUP_ON_EXIT);
    if(pid == -1){
        //  printf("> [Error] failed to create the process: %s\n",file_name);
        //  sys_reflush();
    }else{
        //  printf("succeed to create the process: %s, the pid: %d\n",file_name, pid);
        //  sys_reflush();
        return pid;
    }
}


pid_t do_command(char *buffer, int length){
    /*考虑最多只有六个参数*/
    char cmd_two_dim[15][40] = {0};
    int i;
    /*得到以上所有的参数*/
    /*上一个命令的截止地址*/
    int index = 0;
    /*判断是第几个参数*/
    int judge = 0;
    // judge for str
    int judge_str = 0;
    for (i = 0; i < length; i++){
        char ch = buffer[i];
        if (ch == '\"' && !judge_str)
        {
            judge_str = 1;
            continue;
        } else if (ch == '\"' && judge_str)
        {
            judge_str = 0;
            continue;
        }
        if (ch == ' ' && !judge_str){
            cmd_two_dim[judge++][index++] = 0;
            index = 0;
        } 
        else      
            cmd_two_dim[judge][index++] = ch;    
    }
    // memcpy()
    cmd_two_dim[judge][index] = 0;
    pid_t pid;
    /* only exec */
    uint32_t exec = 0;
    if(!strcmp(cmd_two_dim[0], "exec")) exec = 1;
    else {
        printf("Unknown command\n");
        return 0;
    }
    if (!strcmp(cmd_two_dim[1], "echo"))
    {
        printf("%s\n", &buffer[10]);
        return 0;
    }
        
    if (!strcmp(cmd_two_dim[2], "hwclock"))
        return 0;
    final_test_t final_test = {0};
    final_test.argc = judge;
    for (int i = 0; i < MAX_ARGV_NUM; i++)
    {
        final_test.argv[i] = cmd_two_dim[i + 1];
    }
    
    if(exec){
        pid = shell_exec(&final_test);
    }
    return pid;
}

void get_name(char *buffer, int length , char * name)
{
    /*考虑最多只有六个参数*/
    char cmd_two_dim[15][40] = {0};
    int i;
    /*得到以上所有的参数*/
    /*上一个命令的截止地址*/
    int index = 0;
    /*判断是第几个参数*/
    int judge = 0;
    // judge for str
    int judge_str = 0;
    for (i = 0; i < length; i++){
        char ch = buffer[i];
        if (ch == '\"' && !judge_str)
        {
            judge_str = 1;
            continue;
        } else if (ch == '\"' && judge_str)
        {
            judge_str = 0;
            continue;
        }
        if (ch == ' ' && !judge_str){
            cmd_two_dim[judge++][index++] = 0;
            index = 0;
        } 
        else      
            cmd_two_dim[judge][index++] = ch;    
    }
    // memcpy()
    cmd_two_dim[judge][index] = 0;
    strcpy(name, cmd_two_dim[2]);
}



int main()
{
    // TODO:
    printf("> info: the beginning of the OScomp!\n");
    /**
     * @brief 如果想要在QEMU里面运行就得以一下的格式，并且要将相应的elf编译到内核当中
     */
    int r = 1;
    char name[20];
    // ============================= for lua ==================================
    printf("> info: pre load lua\n");
    sys_pre_load("lua", LOAD);
    // for lua_test
    for (int i = 0; lua_test[i] != NULL; i++)
    {
        printf("%d: %s\n", r++, lua_test[i]);
        char *name;
        pid_t pid = do_command(lua_test[i], strlen(lua_test[i]));
        if (pid)
            waitpid(pid);
        printf("testcase lua %s success\n", &lua_test[i][9]);
    }  
    printf("> info: pre free lua\n");
    sys_pre_load("lua", FREE); 

    // // ============================= for lmbench ==================================
    printf("> info: pre load busybox\n");
    sys_pre_load("busybox", LOAD);
    // // ============================= for busybox ==================================
    for (int i = 0; busybox_test[i] != NULL; i++)
    {
        printf("%d: %s\n", r++, busybox_test[i]);
        pid_t pid = do_command(busybox_test[i], strlen(busybox_test[i]));
        if (pid)
            waitpid(pid);
        printf("testcase busybox %s success\n", &busybox_test[i][13]);
    }
    
    // ============================= for pre_lmbench_all ==========================
    for (int i = 0; pre_lmbench_test[i] != NULL; i++)
    {
        printf("%d: %s\n", r++, pre_lmbench_test[i]);
        pid_t pid = do_command(pre_lmbench_test[i], strlen(pre_lmbench_test[i]));
        if (pid)
            waitpid(pid);
    }
    printf("> info: pre load lmbench_all\n");
    sys_pre_load("lmbench_all", LOAD);
    // sys_pre_load("busybox", FREE);
    // sys_pre_load("busybox", FREE);
    // for lmbench test
    // uint64_t page_num =  extend(100, LOAD);
    // printf("extend used page num: %d\n", page_num);
    int flag = 0;
    for (int i = 0; lmbench_test[i] != NULL; i++)
    {
        get_name(lmbench_test[i], strlen(lmbench_test[i]), name);
        printf("%d: %s\n", r++, lmbench_test[i]);
        if (!strcmp(name, "lat_ctx") && flag == 0)
        {
            extend(100, LOAD, 0);
            flag = 1;
        } else if (!strcmp(name, "lat_ctx") && flag == 1)
        {
            extend(100, LOAD, 256 + 128);
            flag = 2;           
        } else if (!strcmp(name, "lat_ctx") && flag == 2)
        {
            extend(100, LOAD, 512 + 128);
            flag = 3;           
        }        

        pid_t pid = do_command(lmbench_test[i], strlen(lmbench_test[i]));


        if (pid)
            waitpid(pid);
            
        if (!strcmp(lmbench_test[i], "exec lmbench_all lat_proc -P 1 shell"))
            sys_pre_load("busybox", FREE);
    }  
    if (!strcmp(name, "lat_ctx") && flag)
    {
        extend(100, FREE, 0);
        flag = 0;
    }
    // extend(100, FREE);
    printf("> info: pre free lmbench\n");
    sys_pre_load("lmbench_all", FREE);


    printf("!TEST FINISH!\n");   
    for(int i = 0; ; i++){
        // printf("the ticks :%ld %ld\n", sys_get_tick(), sys_get_timer());
        // printf("> hello world! %d\n", i);
        sys_sleep(1);
        // printf("the ticks :%ld %ld\n", sys_get_tick(), sys_get_timer());
    }
        
    return 0;
}