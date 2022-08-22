#include <pgtable.h>
#include <swap/swap.h>
#include <os/mm.h>
LIST_HEAD(swapPageList)
uint64_t swap_num = 0;
uint64_t swap_offset = 4;
uint64_t threshold = 0;
/**
 * @brief do clock_algorithm to find a suitable page to swap to SD
 * and return her kva
 * @return return the page_node 
 */
page_node_t * clock_algorithm ()
{
    if (clock_pointer == NULL)
        clock_pointer = swapPageList.next;
    if (list_empty(&swapPageList)) assert(0);
    // 1. find one page to swap here
    page_node_t * swapPage = find_swap_page();
    
    if (swapPage->list.next == &swapPageList)
        clock_pointer = swapPage->list.next->next;
    else 
        clock_pointer = swapPage->list.next;
    

    // 2. now we found the swap page, we will write it to the pointed sector
    // the sector num in the SD
    uint64_t w_sec;
    if (check_attribute(swapPage->page_pte, _PAGE_DIRTY))
    {
        // the page has been write, we have to write it to SD
        w_sec = swapPage->sd_sec ? swapPage->sd_sec : get_swap_sec();

        disk_write_swap(swapPage->kva_begin, w_sec + SD_SWAP_SPACE, SEC_NUM_PAGE);
    } else 
    {
        // the page not been write, if it has been swap to SD, we don't need to 
        if (!swapPage->sd_sec)
        {
            // write to SD first
            w_sec = get_swap_sec();
            
            disk_write_swap(swapPage->kva_begin, w_sec + SD_SWAP_SPACE, SEC_NUM_PAGE);
        } else w_sec = swapPage->sd_sec;
    }
    // 3. save the w_sec on the pfn of the sector
    set_pfn(swapPage->page_pte, w_sec);
    // 4. clear the _PAGE_PRESENT of the swap page pte
    clear_attribute(swapPage->page_pte, _PAGE_PRESENT);
    // 5. set the SD flag
    add_attribute(swapPage->page_pte, _PAGE_SD);
    
    // now, we finished all the task of the old page 
    return swapPage;
}


// we will find the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0 page
page_node_t * find_swap_AD()
{
    list_node_t* entry = clock_pointer;
    list_node_t* q = clock_pointer;
    page_node_t *page_entry;
    page_node_t *swapPage = NULL;
    do
    {
        page_entry = list_entry(q, page_node_t, list);
        // find _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0
        if (page_entry->page_pte == NULL) goto goon;
        if (!check_attribute(page_entry->page_pte, _PAGE_ACCESSED | _PAGE_DIRTY) && \
             check_attribute(page_entry->page_pte, _PAGE_WRITE))
        {
            swapPage = page_entry;
            return swapPage;
        } 
goon:   ;
        // we can't stand in the list_head swapPageList
        if (page_entry->list.next == &swapPageList)
            q = page_entry->list.next->next;
        else
            q = page_entry->list.next;
    } while (q != entry);    

    return NULL;       
}

// we will find the _PAGE_ACCESSED == 0
page_node_t * find_swap_A()
{
    list_node_t* entry = clock_pointer;
    list_node_t* q = clock_pointer;
    page_node_t *page_entry;
    page_node_t *swapPage = NULL;
    do
    {
        page_entry = list_entry(q, page_node_t, list);
        if (page_entry->page_pte == NULL) goto goon;
        // find _PAGE_ACCESSED == 0
        if (!check_attribute(page_entry->page_pte, _PAGE_ACCESSED) && \
             check_attribute(page_entry->page_pte, _PAGE_WRITE))
        {
            swapPage = page_entry;
            return swapPage;
        } else
            clear_attribute(page_entry->page_pte, _PAGE_ACCESSED);
goon:
        // we can't stand in the list_head swapPageList
        if (page_entry->list.next == &swapPageList)
            q = page_entry->list.next->next;
        else
            q = page_entry->list.next;
    } while (q != page_entry);

    return NULL;   
}

// clock algorithm find one page to swap out
page_node_t * find_swap_page()
{
    page_node_t *swapPage;
    
    // first find the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0
    if ((swapPage = find_swap_AD()) != NULL)
        return swapPage;

    // second find the _PAGE_ACCESSED == 0
    if ((swapPage = find_swap_A()) != NULL)
        return swapPage;

    // third find the _PAGE_ACCESSED == 0 and _PAGE_DIRTY == 0 again
    if ((swapPage = find_swap_AD()) != NULL)
        return swapPage;    

    // final find the _PAGE_ACCESSED == 0
    if ((swapPage = find_swap_A()) != NULL)
        return swapPage;
    
    // we always find one
    assert(swapPage != NULL);
}