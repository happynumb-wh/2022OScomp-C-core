#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/string.h>
#include <fs/file.h>
#include <pgtable.h>
#include <assert.h>

ptr_t memCurr = FREEMEM;
ptr_t heapCurr = FREEHEAP;

// memory managemanet
page_node_t pageRecyc[NUM_REC];
// control of the free page
source_manager_t freePageManager;


uint64_t krand()
{
    uint64_t rand_base = get_ticks();
    long long tmp = 0x5deece66dll * rand_base + 0xbll;
    uint64_t result = tmp & 0x7fffffff;
    return result;
}

// init mem
uint32_t init_mem(){
    uint64_t memBegin = memCurr;
    // init list
    init_list(&freePageManager.free_source_list);
    init_list(&freePageManager.wait_list);
    for (int i = 0; i < NUM_REC; i++){
        list_add_tail(&pageRecyc[i].list, &freePageManager.free_source_list);
        pageRecyc[i].kva_begin = memBegin;
        pageRecyc[i].share_num = 0;
        pageRecyc[i].is_share = 0;
        memBegin += NORMAL_PAGE_SIZE;
    }

    freePageManager.source_num = NUM_REC;
    return NUM_REC;
}

// alloc one page, return the top of the page
ptr_t allocPage() 
{
    // TODO:
    list_head *freePageList = &freePageManager.free_source_list;
    pcb_t * current_running = get_current_running();
try:    if (!list_empty(freePageList)){
        list_node_t* new_page = freePageList->next;
        page_node_t *new = list_entry(new_page, page_node_t, list);
        /* one use */
        new->share_num++;
        list_del(new_page);
        list_add_tail(new_page, &usedPageList);

        freePageManager.source_num--;
        kmemset(new->kva_begin,0,PAGE_SIZE);
        // printk("alloc mem kva 0x%lx\n", new->kva_begin);
        return new->kva_begin + PAGE_SIZE;        
    }else{
        // memCurr += PAGE_SIZE;    
        printk("Failed to allocPage, I am pid: %d\n", current_running->pid);
        do_block(&current_running->list, &freePageManager.wait_list);
        goto try;
    }
}

// free one page, get the base addr of the page
void freePage(ptr_t baseAddr)
{
    // printk("freePage:%lx\n",baseAddr);
    /* alloc page */
    page_node_t *recycle_page = &pageRecyc[GET_MEM_NODE(baseAddr)];
    /* the page has been shared */
    if (--(recycle_page->share_num) != 0) {
        return;
    }
    list_head *freePageList = &freePageManager.free_source_list;    
    list_del(&recycle_page->list);
    list_add(&recycle_page->list, freePageList);
    freePageManager.source_num++;
    // we only free one
    if (!list_empty(&freePageManager.wait_list))
        do_unblock(freePageManager.wait_list.next);
}

// don't care the 
void freePage_voilence(ptr_t baseAddr)
{
    /* alloc page */
    page_node_t *recycle_page = &pageRecyc[GET_MEM_NODE(baseAddr)];
    recycle_page->share_num = 0;
    list_head *freePageList = &freePageManager.free_source_list;    
    list_del(&recycle_page->list);
    list_add_tail(&recycle_page->list, freePageList);
    freePageManager.source_num++;
    if (!list_empty(&freePageManager.wait_list))
        do_unblock(freePageManager.wait_list.next);
}

/* malloc a place in the mem, return a place */
void *kmalloc(size_t size)
{
    if (size != NORMAL_PAGE_SIZE) {
        assert(0);
        ptr_t ret = ROUND(heapCurr, 8);
        heapCurr = ret + size;        
        return (void*)ret;
    } else {
        return (void*)(allocPage() - NORMAL_PAGE_SIZE);
    } 
}

/* free a mem */
void kfree(void * size){
    if ((ptr_t)size > FREEHEAP) ; // nrver recycle
    else if ((ptr_t)size < FREEMEM) ;// null or less
    else {
        freePage((ptr_t)size);
    }
}

/*  */
extern void kfree_voilence(void * size)
{
    if ((ptr_t)size > FREEHEAP) ; // nrver recycle
    else if ((ptr_t)size < FREEMEM) ;// null or less
    else {
        freePage_voilence((ptr_t)size);
    }   
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    kmemcpy((char *)dest_pgdir, (char *)src_pgdir, PAGE_SIZE);
}

/**
 * @brief Check whether the page exists
 * if not , alloc it and set the flag
 * return the pagebase
 */
PTE check_page_set_flag(PTE* page, uint64_t vpn, uint64_t flag){
    if (page[vpn] & _PAGE_PRESENT) 
    {
        /* valid */
        return pa2kva((get_pfn(page[vpn]) << NORMAL_PAGE_SHIFT));
    } else
    {
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&page[vpn], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to set the U, the kernel will not set the U 
         * which means that the User will not get it, but we will 
         * temporary set it as the User. so the U will be high
        */ 
        set_attribute(&page[vpn], flag);
        
        return newpage; 
    }

}

/**
 * @brief for the clone to add one more share the phyc page
 * no return;
 */
void share_add(uint64_t baseAddr){
    pageRecyc[GET_MEM_NODE(baseAddr)].share_num++;
    return;
}
/**
 * @brief for the clone share_num -- 
 * no return 
 */
void share_sub(uint64_t baseAddr){
    pageRecyc[GET_MEM_NODE(baseAddr)].share_num--;
    return;  
}

uintptr_t shm_page_get(int key)
{
    // TODO(c-core): 
    pcb_t * current_running = get_current_running();
    if(!list_empty(&shareMemKey)){

        list_node_t * pointer = shareMemKey.next;
        do{
            share_page * find = list_entry(pointer, share_page, list);
            if(find->key == key){
                uintptr_t vta;
repeat:         vta = krand() % 0xf0000000;
                vta &= ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
                if(!get_kva_of(vta, current_running->pgdir))
                    goto repeat;
                else{
                    alloc_page_point_phyc(vta, current_running->pgdir, find->kva, MAP_USER);
                    find->visted++;
                    return vta ;
                }   
            }
            pointer = pointer->next;
        } while (pointer != &shareMemKey);
    }
    /* alloc new */
    share_page * new = (share_page *)kmalloc(sizeof(share_page));
    new->key = key;
    uintptr_t kva;
    uintptr_t vta;
new:vta = krand() % 0x10000000;
    vta &= ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
    if(!get_kva_of(vta, current_running->pgdir))
        goto new;
    else
        kva = alloc_page_helper(vta, current_running->pgdir, MAP_USER);   
    new->kva = kva;
    new->visted = 1;
    list_add(&new->list, &shareMemKey);
    return vta;    
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
    pcb_t * current_running = get_current_running();
    uintptr_t kva = get_pfn_of(addr, current_running->pgdir);
    if(!list_empty(&shareMemKey)){
        list_node_t * pointer = shareMemKey.next;
        do{
            share_page * find = list_entry(pointer, share_page, list);
            if(find->kva == kva){
                if((--(find->visted)) == 0){
                    list_del(&find->list);
                    (*get_PTE_of(addr, current_running->pgdir)) = 0;
                    freePage(find->kva);
                }
            }
            pointer = pointer->next;
        } while (pointer != &shareMemKey);        
    } else {
        prints("> [Error]\n");
        while (1);
    }
}


/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, uint64_t mode)
{
    // TODO:
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    
    second_page = (PTE *)check_page_set_flag(page_base, vpn[2], _PAGE_PRESENT);
    /* third page */
    third_page = (PTE *)check_page_set_flag(second_page, vpn[1], _PAGE_PRESENT);

    /* final page */
    /* the R,W,X == 1 will be the leaf */
    uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
                        | _PAGE_EXEC  | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          _PAGE_USER);   
    /* final page */
    uint64_t final_page = check_page_set_flag(third_page, vpn[0], pte_flags) | (va & (uint64_t)0x0fff);

    // just for the status ctx
    if (status_ctx)
    {
        page_node_t * final_node = &pageRecyc[GET_MEM_NODE(final_page)];
        final_node->sd_sec = 0;
        final_node->page_pte = &third_page[vpn[0]];
    }

    return final_page;

}

/* alooc the va to the pointer */
PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint64_t mode){
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    second_page = (PTE *)check_page_set_flag(page_base, vpn[2], _PAGE_PRESENT);

    /* third page */
    third_page = (PTE *)check_page_set_flag(second_page, vpn[1], _PAGE_PRESENT);
    /* final page */
    // if((third_page[vpn[0]] & _PAGE_PRESENT) != 0)
    //     /* return the physic page addr */
    //     return 0;//pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    set_pfn(&third_page[vpn[0]], kva2pa(kva) >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
                        | _PAGE_EXEC  | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          _PAGE_USER);
    set_attribute(&third_page[vpn[0]], pte_flags);
    if (status_ctx)
    {
        page_node_t * final_node = &pageRecyc[GET_MEM_NODE(kva)];
        final_node->sd_sec = 0;
        final_node->page_pte = &third_page[vpn[0]];
    }    
    return &third_page[vpn[0]];     
}

// uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir){
//     uint64_t vpn[] = {
//                     (vta >> 12) & ~(~0 << 9), //vpn0
//                     (vta >> 21) & ~(~0 << 9), //vpn1
//                     (vta >> 30) & ~(~0 << 9)  //vpn2
//                     };
//     /* the PTE in the first page_table */
//     PTE *page_base = (uint64_t *) pgdir;
//     /* second page */
//     PTE *second_page = NULL;
//     /* finally page */
//     PTE *third_page = NULL;
//     /* find the second page */
//     if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0)//unvalid
//     {
//         return 1;
//     }
//     else
//     {
//         /* get the addr of the second_page */
//         second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
//     }
    
//     /* third page */
//     if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0 )//unvalid or the leaf
//     {
//         return 1;
//     }
//     else
//     {
//         third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
//         /* the va's page */
//     }

//     /* final page */
//     if((third_page[vpn[0]] & _PAGE_PRESENT) == 0)
//         return 1;
//     else
//         return 0;
// }

/* check the physic page in only without AD or in the SD */
uint32_t check_W_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode){
    uint64_t vpn[] = {
                    (va >> 12) & ~(~0 << 9), //vpn0
                    (va >> 21) & ~(~0 << 9), //vpn1
                    (va >> 30) & ~(~0 << 9)  //vpn2
                    };
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL; 
    /* physic page */
    PTE *physic_page = NULL;
    if (((page_base[vpn[2]]) & _PAGE_PRESENT)!=0)
    {
        page_base[vpn[2]] |= _PAGE_ACCESSED; //| (mode == STORE ? _PAGE_DIRTY: 0);
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return NO_ALLOC;

    if (((second_page[vpn[1]]) & _PAGE_PRESENT)!=0)
    {
        second_page[vpn[1]] |= _PAGE_ACCESSED;//| (mode == STORE ? _PAGE_DIRTY: 0);
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return NO_ALLOC; 

    if(((third_page[vpn[0]]) & _PAGE_SD)!=0)
    {
        // third_page[vpn[0]] &= ~_PAGE_SD;
        if((third_page[vpn[0]] & _PAGE_WRITE) == 0 && mode == STORE){
            third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY;
            return IN_SD_NO_W;
        }
        third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0);
        return IN_SD;
    }

    if (((third_page[vpn[0]]) & _PAGE_PRESENT)!=0)
    {
        if((third_page[vpn[0]] & _PAGE_WRITE) == 0 && mode == STORE){
            third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED |  _PAGE_DIRTY;
            return NO_W;
        }
        /* nothing to do */
        if ((mode == LOAD && (third_page[vpn[0]] & _PAGE_ACCESSED)) || \
            (mode == STORE && (third_page[vpn[0]] & (_PAGE_ACCESSED | _PAGE_DIRTY))))
            return -1;
        third_page[vpn[0]] |= _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0);
    }    
    else{
            return NO_ALLOC;
    }
    return ALLOC_NO_AD;
}

uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO:
    pcb_t *current_running = get_current_running();
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if(page_base[vpn[2]] & _PAGE_PRESENT == 0){
        return -1;
    }
    second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    if(second_page[vpn[1]] & _PAGE_PRESENT == 0){
        return -1;
    }
    /* third page */
    third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    freePage(get_kva_of(va, pgdir));
    third_page[vpn[0]] = 0;  
    return 0;
}

/**
 * @brief unmaped write back
 * @param start the mem satrt
 * @param len the write back length
 * @param nfd the fd
 * @param pgdir the padir
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
uintptr_t unmap_write_back(void * start, uint64_t len, fd_t *nfd, uintptr_t pgdir)
{
    uint64_t end = (uint64_t)start + len;
    uint64_t begin = (uint64_t)start;
    for (void *cur_page = begin; cur_page < end; cur_page += NORMAL_PAGE_SIZE)
    {
        // we only write back dirty page
        PTE * pte = get_PTE_of(cur_page, pgdir);
        if (pte == NULL) continue;
        if (*pte & _PAGE_DIRTY)
        {
            fat32_lseek(nfd->fd_num, \
                        nfd->mmap.off + ((uint64_t)cur_page - begin), \
                        SEEK_SET);
            fat32_write(nfd->fd_num, cur_page, MIN(end - (uint64_t)cur_page, NORMAL_PAGE_SIZE));
        }
    }
    return 0;
}


void *visit_command_line()
{
    pcb_t * current_running = get_current_running();

    return (void *)(current_running->user_stack_top + 8);
}