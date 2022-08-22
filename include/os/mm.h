#ifndef MM_H
#define MM_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
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
#include <os/spinlock.h>
#include <os/source.h>
#include <fs/file.h>
#include <type.h>
#include <pgtable.h>
#include "os/list.h"
/* used pagelist */
static LIST_HEAD(usedPageList);
/* shareMemKay */
static LIST_HEAD(shareMemKey);
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define MAP_KERNEL 1
#define MAP_USER 2
#define LOAD 0
#define STORE 1
#define NO_ALLOC 0
#define ALLOC_NO_AD 1
#define IN_SD 2
#define IN_SD_NO_W 3
#define NO_W 4

#define MAX_PAGE_NUM 12

// First 1 MB for the kernel boot
#ifdef k210
    #define INIT_KERNEL_STACK 0xffffffc080100000lu
    #define INIT_KERNEL_STACK_MSTER 0xffffffc080101000lu
    #define INIT_KERNEL_STACK_SLAVE 0xffffffc080102000lu
#else 
    #define INIT_KERNEL_STACK 0xffffffc080300000lu
    #define INIT_KERNEL_STACK_MSTER 0xffffffc080301000lu
    #define INIT_KERNEL_STACK_SLAVE 0xffffffc080302000lu
#endif

// pcb user stack message
#define USER_STACK_ADDR 0xf00010000lu
#define SIGNAL_HANDLER_ADDR 0xf10010000lu
#define USER_STACK_HIGN 0xf00000000lu
// alloc high 0.5MB for the Heap, never be realloc
#define FREEHEAP (KERNEL_END)//Heap
// Free MEM
#define FREEMEM (INIT_KERNEL_STACK + 2 * PAGE_SIZE)
// the 6.5MB for the kernel
#define NUM_REC ((FREEHEAP - FREEMEM) / NORMAL_PAGE_SIZE)

#define GET_MEM_NODE(baseAddr) (((baseAddr) - (FREEMEM)) / NORMAL_PAGE_SIZE)

/* the struct for per page */
#pragma(8)
typedef struct page_node{
    list_node_t list;
    volatile PTE * page_pte;
    uint64_t kva_begin; /* the va of the page */
    uint64_t sd_sec;    /* last time in SD place */
    uint8_t share_num;  /* share page number */
    uint8_t is_share;   /* maybe for the shm_pg_get */
    
}page_node_t;
#pragma()

/* manager free page */
extern source_manager_t freePageManager;

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))

#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

extern ptr_t memCurr;
extern list_node_t bioPageList;

/* 记录回收的页的信息 */
#pragma(8)
typedef struct {
    // uintptr_t pa;       //真实地址
    uintptr_t kva;       //虚拟地址
    list_node_t list;   //拉链
}free_page_t;
#pragma()

/* 记录进程持有页的信息 */
#pragma(8)
typedef struct {
    uintptr_t vta;       //虚拟地址
    uintptr_t SD_place;  //存储在磁盘的哪一个位置
    list_node_t list;    //拉链
}page_t;
#pragma()

#pragma(8)
typedef struct{
    uintptr_t kva;       //虚拟地址
    // uintptr_t kva;       //物理页的虚拟地址
    uint32_t visted;     //持有改物理页面的进程数
    uint32_t  key;       //key
    list_node_t list;    //拉链
} share_page;
#pragma()

extern page_node_t pageRecyc[NUM_REC];

extern uint32_t init_mem();
extern uint64_t krand();
/* dynamic malloc */
extern ptr_t allocPage();
extern void freePage(ptr_t baseAddr);
extern void freePage_voilence(ptr_t baseAddr);

extern void* kmalloc(size_t size);
extern void kfree(void * size);
extern void kfree_voilence(void * size);

extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
/* alloc a new page */
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, uint64_t mode);
/* alloc a page pointer phyc */
extern PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint64_t mode);

/* check weather the va has been used*/
// extern uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir);
/* check the error type */
extern uint32_t check_W_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode);

/* share mem get */
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);
/* check the page and set flag for it */
PTE check_page_set_flag(PTE* page, uint64_t vpn, uint64_t flag);

/* free pgtable */
uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir);
/* mmap finish write_back */
uintptr_t unmap_write_back(void * start, uint64_t len, fd_t *nfd, uintptr_t pgdir);


/* recycle */
int recycle_page_default(uintptr_t pgdir);
int recycle_page_voilent(uintptr_t pgdir);
int recycle_page_part(uintptr_t pgdir, void *exe_load, uint64_t edata);

/* change the data  */
uint64_t do_brk(uintptr_t ptr);
uint64_t lazy_brk(uintptr_t ptr);
int do_mprotect(void *addr, size_t len, int prot);
int do_madvise(void* addr, size_t len, int notice);
int do_membarrier(int cmd, int flags);
/* deal the mmirq with clone no W set */
void deal_no_W(uint64_t fault_addr);
void deal_no_alloc(uint64_t fault_addr, uint64_t mode);
void deal_in_SD(uint64_t fault_addr);
/* alloc_page_helper addr ok */ 
uint8 is_legal_addr(uint64_t fault_addr);
void share_add(uint64_t baseAddr);
void share_sub(uint64_t baseAddr);
// membarrier
int do_membarrier(int cmd, int flags);

void load_lazy_mmap(uint64_t fault_addr, uint64_t kva);

void *visit_command_line();


#endif /* MM_H */
