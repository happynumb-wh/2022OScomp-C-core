#ifndef SPECIAL_CTX_H
#define SPECIAL_CTX_H

#include <type.h>
#include <os/mm.h>
#define MAX_EXTEND (max_extend)
#define PIPE_ARRAY 200

#define PIPE_READ 1
#define PIPE_WRITE 2
// my_pipe struct for ctx
// don't revamp it !!!
typedef struct ctx_pipe
{
    uint8_t used;
    uint8_t dev;
    // actually the pipe_num
    uint8_t fd_num;
    // 0 for read, 1 for write
    uint8_t pipe_type;
}ctx_pipe_t;

// don't revamp it !!!
typedef struct my_pipe
{
    list_head list;
    ctx_pipe_t pipe_array[PIPE_ARRAY];
}my_pipe_t;

// the list save the alloc page
extern list_node_t extendPageList;
// =================== maybe unuseful ==================
// the list save the alloc Pcb
extern list_node_t extendPcbList;
// the list save the alloc extend FdList
extern list_node_t extendFdList;

// the extend status
extern uint64_t status_ctx;
extern uint64_t max_extend;
// save 


// extend the pcb and fd to point num
uint64_t extend_pcb_fd(uint64_t num);
// close the extend pcb and fd
uint64_t close_extend(uint64_t num);
// do 
#define EXTEND_FREE 0
#define EXTEND_LOAD 1

uint64_t do_extend(uint64_t num, int how, uint64_t threold);

// alloc one page and add to extendPageList
static inline uint64_t alloc_extend_page()
{
    uint64_t page = kmalloc(PAGE_SIZE);
    kmemset(page, 0, PAGE_SIZE);
    page_node_t * new_page = &pageRecyc[GET_MEM_NODE(page)];
    list_del(&new_page->list);
    list_add_tail(&new_page->list, &extendPageList);
    
    return page;
}

// tools like file.c
void init_extend_fd(ctx_pipe_t * cpt);
// copy 
void copy_extend_fd(ctx_pipe_t * dst_cpt, ctx_pipe_t * src_cpt);
// get one cpt index
uint32_t get_one_cpt(void * pcb);
// write the cpt pipe
int write_cpt(ctx_pipe_t * nfd, uchar *buf, size_t count);
// read the cpt pipe
int read_cpt(ctx_pipe_t * nfd, uchar *buf, size_t count);

#endif