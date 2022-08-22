#ifndef SWAP_H
#define SWAP_H
#include <type.h>
#include <os/mm.h>
#include <fs/fat32.h>
/* used page threshold */
// when the page_num in the kernel less than 128(0.5MB), we will begin page_swap
#ifndef k210
    #define THRESHOLD_PAGE_NUM (threshold)
#else
    #define THRESHOLD_PAGE_NUM (threshold)
#endif

#define SEC_NUM_PAGE (PAGE_SIZE / SECTOR_SIZE)

/* the beginning of swap sector in SD  */
#ifndef k210
    #define SD_SWAP_SPACE 0
#else
    #define SD_SWAP_SPACE (fat.first_data_sec + 10000)
#endif
/* the offset of the swap sector */
// static uint64_t swap_offset = 0;
extern uint64_t swap_offset;

/* the page join the swap */ 
// static LIST_HEAD(swapPageList);
extern list_head swapPageList;
// static uint64_t swap_num = 0;
extern uint64_t swap_num;
// threshold
extern uint64_t threshold;

// static page_node_t swapPageList;

static list_node_t *clock_pointer = NULL;

// do clock_algorithm to find a suitable page to swap to SD
page_node_t * clock_algorithm ();

// find one page to swap to SD
// we will find the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0 page
// second the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 1
page_node_t * find_swap_page();

// find the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0 page
page_node_t * find_swap_AD();

// find the _PAGE_ACCESSED == 0 
page_node_t * find_swap_A();

// return a sec num to save swap page
static inline uint32_t get_swap_sec()
{
    uint64_t save = swap_offset;
    swap_offset += SEC_NUM_PAGE;
    return save;
}

// get the SD sector from va
static inline uint64_t get_swap_sd(uint64_t va, uintptr_t pgdir_va)
{
    /**
     * get table kva
     */
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    // first null
    if (page_base[vpn[2]] == NULL)
        return NULL;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    // second null
    if (second_page[vpn[1]] == NULL)
        return NULL;    
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    /* third null */
    if (third_page[vpn[0]] == NULL)
        return NULL;    
    return get_pfn(third_page[vpn[0]]);       
}


#endif