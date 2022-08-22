#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <swap/swap.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>

/**
 * @brief for the clone copy on write 
 * 
 */
void deal_no_W(uint64_t fault_addr)
{
    // printk("deal no W: 0x%lx\n", fault_addr);
    pcb_t *current_running = get_current_running();
    /* the fault pte */
    PTE *fault_pte = get_PTE_of(fault_addr, current_running->pgdir);
    /* fault kva */
    uint64_t fault_kva = pa2kva((get_pfn(*fault_pte) << NORMAL_PAGE_SHIFT));
    // the fault base
    page_node_t * fault_page_node = &pageRecyc[GET_MEM_NODE(fault_kva)];

    if (pageRecyc[GET_MEM_NODE(fault_kva)].share_num == 1) {
        // the page belong me 
        if (status_ctx)
            fault_page_node->page_pte = fault_pte;
        set_attribute(fault_pte, get_attribute(*fault_pte) | _PAGE_WRITE);
        return;
    }
    /* share_num --  */
    share_sub(fault_kva);    
    // ======================== for swap and ctx ================================
    if (status_ctx && freePageManager.source_num < THRESHOLD_PAGE_NUM)
    {
        // TODO
        page_node_t * swapPage = clock_algorithm();
        // clear
        if (fault_page_node->page_pte == fault_pte)
        {
            fault_page_node->page_pte = NULL;
        }
        assert(swapPage != NULL)
        // clear page
        // clear_pgdir(swapPage->kva_begin);
        share_pgtable(swapPage->kva_begin, fault_kva);
        // printk("need to swapPage for copy on write!!!\n");
        alloc_page_point_phyc(fault_addr, \
                  current_running->pgdir, \
                  swapPage->kva_begin, \
                  MAP_USER);
        // set new page
        add_attribute(swapPage->page_pte, _PAGE_DIRTY);
        
        return;        
    }
    // ======================== for swap and ctx ================================
    
    /* alloc a new page for current */
    uintptr_t new_page = allocPage() - PAGE_SIZE;

    // ======================== for swap and ctx ================================
    if (status_ctx)
    {
        page_node_t * page = &pageRecyc[GET_MEM_NODE(new_page)];
        list_del(&page->list);
        list_add_tail(&page->list, &swapPageList);
        page->sd_sec = 0; 
        page->page_pte = fault_pte;     
        // add_attribute(page->page_pte, _PAGE_DIRTY);  
    }
    // ======================== for swap and ctx ================================
    /* copy on write */
    share_pgtable(new_page, fault_kva);
    /* set pfn */
    set_pfn(fault_pte, kva2pa(new_page) >> NORMAL_PAGE_SHIFT);
    /* set attribute */
    set_attribute(fault_pte, get_attribute(*fault_pte) | (_PAGE_WRITE | _PAGE_DIRTY));
}   

/**
 * @brief  deal the no alloc page fault
 * @param fault_addr the user addr
 * 
 */
void deal_no_alloc(uint64_t fault_addr, uint64_t mode)
{
    pcb_t * current_running = get_current_running();
    if (freePageManager.source_num < THRESHOLD_PAGE_NUM && status_ctx)
    {
        // TODO
        page_node_t * swapPage = clock_algorithm();
        assert(swapPage != NULL)
        // clear page
        clear_pgdir(swapPage->kva_begin);
        // printk("need to swapPage!!!\n");
        // while (1); 
        alloc_page_point_phyc(fault_addr, \
                  current_running->pgdir, \
                  swapPage->kva_begin, \
                  MAP_USER);
        if (mode == STORE)
            add_attribute(swapPage->page_pte, _PAGE_DIRTY);
        else 
            add_attribute(swapPage->page_pte, _PAGE_ACCESSED);
        // reload_lazy_elf(fault_addr, ROUNDDOWN(swapPage->kva_begin, PAGE_SIZE));
        return;
        
    }
    uint64_t kva = alloc_page_helper(fault_addr, current_running->pgdir, MAP_USER);
    if (!status_ctx)
    {
        load_lazy_mmap(fault_addr, ROUNDDOWN(kva, PAGE_SIZE));
    } else 
    {
        // reload_lazy_elf(fault_addr, ROUNDDOWN(kva, PAGE_SIZE));
        // add to swapPageList
        page_node_t * page = &pageRecyc[GET_MEM_NODE(kva)];
        list_del(&page->list);
        list_add_tail(&page->list, &swapPageList);
        if (mode == STORE)
            add_attribute(page->page_pte, _PAGE_DIRTY);
        else 
            add_attribute(page->page_pte, _PAGE_ACCESSED);
    }
}

/**
 * @brief deal the target page in SD
 * @param fault_adrr the fault user addr
 * 
 */
void deal_in_SD(uint64_t fault_addr)
{
    pcb_t * current_running = get_current_running();
    uint64_t sd_sec = get_swap_sd(fault_addr, current_running->pgdir);
    // prints("> Attention: the 0x%lx addr was swapped to SD! the sec_num is %d\n", 
    // fault_addr, sd_sec);    
    if (freePageManager.source_num < THRESHOLD_PAGE_NUM)
    {
        page_node_t *swapPage = clock_algorithm();

        assert(swapPage != 0);
        // clear_pgdir(swapPage->kva_begin);
        // printk("need to swapPage!!!\n");
        // while (1); 
        disk_read_swap(swapPage->kva_begin, sd_sec + SD_SWAP_SPACE, PAGE_SIZE/SECTOR_SIZE);
        alloc_page_point_phyc(fault_addr, \
                  current_running->pgdir, \
                     swapPage->kva_begin, \
                               MAP_USER); 
        swapPage->sd_sec = sd_sec;       
    } else 
    {
        // alloc new page
        // printk("don't need to swapPage!!!\n");
        uint64_t new_kva = allocPage() - PAGE_SIZE;
        disk_read_swap(new_kva, sd_sec + SD_SWAP_SPACE, PAGE_SIZE/SECTOR_SIZE);
        // alloc new page
        alloc_page_point_phyc(fault_addr, \
                  current_running->pgdir, \
                                 new_kva, \
                               MAP_USER); 
        page_node_t *node = &pageRecyc[GET_MEM_NODE(new_kva)];
        node->sd_sec = sd_sec;
    }
    // while(1);
}

/**
 * @brief load_lazy_mmap
 * @param fault_addr the user fault addr
 * @param kva the alloc kernel page
 * 
 */
void load_lazy_mmap(uint64_t fault_addr, uint64_t kva){
    if (!kstrcmp(test_name, "lat_pagefault")) return;
    pcb_t * current_running = get_current_running();
    //search for lazy mmap (if the fault_addr is in some fd's map zone)
    for (int i=0; i<MAX_FILE_NUM; i++) {
        fd_t *nfd = &current_running->pfd[i];
        if (nfd->used && nfd->mmap.used &&
            fault_addr >= (uint64_t)nfd->mmap.start && 
            fault_addr <= (uint64_t)nfd->mmap.len + (uint64_t)nfd->mmap.start){
                
            // save fd's offset
            int old_seek = fat32_lseek(nfd->fd_num, 0, SEEK_CUR);
            void *start = nfd->mmap.start;
            int len = nfd->mmap.len;
            // void *cur_page = ROUNDDOWN(fault_addr,NORMAL_PAGE_SIZE);
            // 1. find the base
            uint64_t left_page = fault_addr - ROUNDDOWN(fault_addr,NORMAL_PAGE_SIZE);
            uint64_t begin; 
            if (fault_addr & 0xfff)
                begin = (fault_addr - (uint64_t)start) > left_page ? ROUNDDOWN(kva,NORMAL_PAGE_SIZE)
                                    : kva - (fault_addr - (uint64_t)start);
            else 
                begin = kva;
            // 2. find the high
            uint64_t mmap_end = (uint64_t)start + len;
            uint64_t end;
            if (fault_addr & 0xfff)
                end = (mmap_end - fault_addr) > ROUND(fault_addr,NORMAL_PAGE_SIZE) - fault_addr ?
                            ROUND(kva,NORMAL_PAGE_SIZE) : kva + (mmap_end - fault_addr);
            else 
                end = kva + MIN(mmap_end - fault_addr, PAGE_SIZE);
            
            fat32_lseek(nfd->fd_num, nfd->mmap.off + (fault_addr - (uint64_t)start), SEEK_SET);
            fat32_read(nfd->fd_num, begin, (end - begin));
            // if it is the first page of mmap
            // if (fault_addr <= ROUND(nfd->mmap.start,NORMAL_PAGE_SIZE)){
            //     fat32_lseek(nfd->fd_num,nfd->mmap.off,SEEK_SET);
            //     fat32_read(nfd->fd_num,nfd->mmap.start,ROUND(nfd->mmap.start,NORMAL_PAGE_SIZE)-(uint64_t)nfd->mmap.start);
            // }
            // // if it's not the first page of mmap
            // else {
            //     uint64_t file_off = ROUNDDOWN(fault_addr,NORMAL_PAGE_SIZE) - (uint64_t)nfd->mmap.start;
            //     fat32_lseek(nfd->fd_num,nfd->mmap.off + file_off,SEEK_SET);
            //     // if it's the last page of mmap
            //     fat32_read(nfd->fd_num,ROUNDDOWN(fault_addr,NORMAL_PAGE_SIZE),
            //                 (ROUNDDOWN(fault_addr,NORMAL_PAGE_SIZE) + NORMAL_PAGE_SIZE >= (uint64_t)(nfd->mmap.start+nfd->mmap.len))
            //                     ? (uint64_t)(nfd->mmap.start+nfd->mmap.len)%NORMAL_PAGE_SIZE : NORMAL_PAGE_SIZE);
            // }

            // restore fd's offset
            fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);
            }
        }
}


/**
 * @brief judge the addr is legal addr
 * @param fault_addr the addr exception
 * @return if legal return 1 else return 0
 */
uint8 is_legal_addr(uint64_t fault_addr)
{
    pcb_t * current_running = get_current_running();
    // for heap
    if (fault_addr < current_running->edata && \
        fault_addr > current_running->elf.edata)
        return 1;

    uintptr_t keep_vaddr_base = current_running->exe_load->phdr[1].p_vaddr;
    uintptr_t keep_vaddr_top = current_running->exe_load->phdr[1].p_vaddr + \
                                    current_running->exe_load->phdr[1].p_memsz;    

    if (fault_addr >= keep_vaddr_base && keep_vaddr_top <= keep_vaddr_top)
        return 1;
    // for stack
    uint64_t sp = current_running->save_context->regs[2];
    if (fault_addr & USER_STACK_HIGN)
        return 1;
    return 0;
}

