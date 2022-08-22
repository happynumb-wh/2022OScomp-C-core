#include <sys/special_ctx.h>
#include <os/sched.h>
#include <fs/file.h>
#include <fs/pipe.h>
#include <swap/swap.h>
#include <assert.h>
#include <utils/utils.h>
#include <screen.h>
// the flag open ctx
uint64_t status_ctx = 0;

uint64_t max_extend;


// the extend page list
LIST_HEAD(extendPageList);
// the extend Pcb list
LIST_HEAD(extendPcbList);
// the extend fd list
LIST_HEAD(extendFdList);
/**
 * @brief extend the pcb num to point num
 * @param num the target num
 * @return used page num
 */
uint64_t extend_pcb_fd(uint64_t num)
{
    uint64_t page_num = 0;
    // 1. open 
    status_ctx = 1;
    max_extend = num;
    // recycle the bio page
    page_node_t * recycle_entry = NULL, *recycle_q;
    list_for_each_entry_safe(recycle_entry, recycle_q, &bioPageList, list)
    {
        kfree_voilence(recycle_entry->kva_begin);
    }

    // 2. extend the pcb
    // one page can save pcb
    // uint8_t PcbPage = PAGE_SIZE / sizeof(pcb_t);
    // // alloc pcb
    // int new_index = 0;
    // for (int i = 0; i < num - NUM_MAX_TASK; i+=PcbPage)
    // {
    //     pcb_t * new = (pcb_t *)alloc_extend_page();
    //     page_num++;
    //     // init the pcb
    //     for (int j = 0; j < PcbPage && (i + j) < num - NUM_MAX_TASK; j++)
    //     {
    //         new[j].used = 0;
    //         new[j].status = TASK_EXITED;
    //         list_add_tail(&new[j].list, &extendPcbList);
    //         // extend_pcb[new_index++] = &new[j];
    //     }
    // }
    // 2. extend the pipe
    // one page can save pipe 
    uint8_t PipePage = PAGE_SIZE / sizeof(my_pipe_t);
    for (int i = 1; i < num; i+=PipePage)
    {
        my_pipe_t * new = (my_pipe_t *)alloc_extend_page();
        page_num++;
        // init the pcb
        for (int j = 0; j < PipePage && (i + j) < num ; j++)
        {
            kmemset(new[j].pipe_array, 0, sizeof(ctx_pipe_t)*200);
            list_add_tail(&new[j].list, &extendFdList);
        }
    }      

    // 3. alloc pipe_fd to existing pcb
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (!list_empty(&extendFdList))
        {
            my_pipe_t * alloc_pipe = list_entry(extendFdList.next, my_pipe_t, list);
            pcb[i].ctx_pipe_array = alloc_pipe->pipe_array;
            list_del(&alloc_pipe->list);
        } else assert(0);
    }

    // while (!list_empty(&extendPcbList))
    // {
    //     if (!list_empty(&extendFdList))
    //     {
    //         pcb_t * alloc_pcb = list_entry(extendPcbList.next, pcb_t, list);
    //         my_pipe_t * alloc_pipe = list_entry(extendFdList.next, my_pipe_t, list);
    //         alloc_pcb->ctx_pipe_array = alloc_pipe->pipe_array;
    //         list_del(&alloc_pipe->list);
    //         list_del(&alloc_pcb->list);
    //         // add to source pcb
    //         list_add_tail(&alloc_pcb->list, &available_queue.free_source_list);
    //         available_queue.source_num ++;
    //     } else assert(0);        
    // }

    return page_num;

}

/**
 * @brief close the extend, we just need to recycle all the page
 * @param num, the num source
 */
uint64_t close_extend(uint64_t num)
{
    // recover the existing pcb
    init_list(&available_queue.free_source_list);

    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].status == TASK_EXITED)
        {
            list_add_tail(&pcb[i].list, &available_queue.free_source_list);
            pcb[i].ctx_pipe_array = NULL;
        }
    }

    page_node_t * recycle_entry = NULL, *recycle_q;
    // free all extend pages
    list_for_each_entry_safe(recycle_entry, recycle_q, &extendPageList, list)
    {
        kfree_voilence(recycle_entry->kva_begin);
    }
    // close it
    status_ctx = 0;

    return 0;
}

/**
 * @brief the main Function
 * @param num the extend num
 * @param how how to do 
 */
uint64_t do_extend(uint64_t num, int how, uint64_t threold)
{

    if (how == EXTEND_FREE)
        // we just need to free the page volience
        return  close_extend(num);
    // we need to extend pcb and fd
    if (status_ctx)
    {
        threshold = threold;
        return 0;
    }
    threshold = threold;
    return extend_pcb_fd(num);
}

/**
 * @brief init the extend fd struct
 * @param cpt the pointer
 * @return no return
 */
void init_extend_fd(ctx_pipe_t * cpt)
{
    cpt[0].dev = DEV_STDIN; cpt[0].fd_num = 0; cpt[0].used = FD_USED;
    cpt[1].dev = DEV_STDOUT;cpt[1].fd_num = 1; cpt[1].used = FD_USED; 
    cpt[2].dev = DEV_STDERR;cpt[2].fd_num = 2; cpt[2].used = FD_USED;    
    for (int i = 3; i < PIPE_ARRAY; i++)
    {
        /* default */
        cpt[i].dev = DEV_DEFAULT;
        cpt[i].fd_num = i;
        cpt[i].used = FD_UNUSED; 
    }    
}

/**
 * @brief copy the extend fd 
 * @param dst_cpt: the dst cpt
 * @param src_cpt: the source cpt
 */
void copy_extend_fd(ctx_pipe_t * dst_cpt, ctx_pipe_t * src_cpt)
{
    for (int i = 0; i < PIPE_ARRAY; i++)
    {
        if (src_cpt[i].used && src_cpt[i].dev == DEV_PIPE){
            if (src_cpt[i].pipe_type == PIPE_READ) pipe[src_cpt[i].fd_num].r_valid++;
            else if (src_cpt[i].pipe_type == PIPE_WRITE) pipe[src_cpt[i].fd_num].w_valid++;
            else assert(0);
        }    
        kmemcpy((void *)&dst_cpt[i], (void *)&src_cpt[i], sizeof(ctx_pipe_t));        
    }
}

/**
 * @brief get one free cpt index
 * @param pcb the pcb (void *)
 */
uint32_t get_one_cpt(void * pcb)
{
    // the init pcb struct
    pcb_t * under_pcb = (pcb_t*)pcb;
    uint32_t fd = -1;
    for (int i = 0; i < PIPE_ARRAY; i++)
    {
        if (under_pcb->ctx_pipe_array[i].used == FD_UNUSED)
        {
            under_pcb->ctx_pipe_array[i].used = FD_USED;
            fd = i;
            break;
        }
    }
    return fd;
}


/**
 * @brief read from sepcial cpt
 * @param nfd the cpt
 * @param buf the buf
 * @param cout the write count
 */
int write_cpt(ctx_pipe_t * nfd, uchar *buf, size_t count)\
{
    if (nfd->dev == DEV_STDIN){
        return write_ring_buffer(&stdin_buf, buf, count);
    }  
    else if (nfd->dev == DEV_STDOUT) {
        // write_ring_buffer(&stdout_buf, buf, count);
        return screen_stdout(DEV_STDOUT, buf, count);
    }
    else if (nfd->dev == DEV_STDERR){
        // return count;
        if (!kstrcmp(buf,"echo: write error: Invalid argument\n")) 
            return kstrlen("echo: write error: Invalid argument\n"); //ignore this invalid info
        return screen_stderror(DEV_STDERR, buf, count);
    }
    else if (nfd->dev == DEV_NULL) return count;
    else if (nfd->dev == DEV_ZERO) return 0;
    else if (nfd->dev == DEV_PIPE) return pipe_write(buf,nfd->fd_num, count);

    return -1;    
}


/**
 * @brief read from sepcial cpt
 * @param nfd the cpt
 * @param buf the buf
 * @param cout the read count
 */
int read_cpt(ctx_pipe_t * nfd, uchar *buf, size_t count)
{
    if(nfd->dev == DEV_STDIN){
        return read_ring_buffer(&stdin_buf, buf, count);
    }
    else if (nfd->dev == DEV_STDOUT){
        return read_ring_buffer(&stdout_buf, buf, count);
    }  
    else if (nfd->dev == DEV_STDERR){
        return read_ring_buffer(&stderr_buf, buf, count);
        // return 0;
    }
    else if (nfd->dev == DEV_NULL) return 0;
    else if (nfd->dev == DEV_ZERO) {
        kmemset(buf,0,count);
        return count;
    }
    else if (nfd->dev == DEV_PIPE) return pipe_read(buf,nfd->fd_num,count);
}