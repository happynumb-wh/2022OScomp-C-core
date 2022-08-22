#include <os/elf.h>
#include <os/mm.h>

// load the connect into the memory
extern uintptr_t load_connector(const char * filename, uintptr_t pgdir)
{
    int elf_id;
    if ((elf_id = find_name_elf(filename)) == -1)
    {
        printk("[Error] fast_load_elf failed to find %s\n", filename);
        return 0;
    }   
    
    // the exe_load
    excellent_load_t * exe_load = &pre_elf[elf_id];
    uint64_t buffer_begin;
    list_node_t * load_node = exe_load->list.next;
    for (int index = 0; index < exe_load->phdr_num; index++)
    {
        uintptr_t dynamic_va_base = exe_load->phdr[index].p_vaddr + DYNAMIC_VADDR_PFFSET;
        for (int i = 0; i < exe_load->phdr[index].p_memsz; i += NORMAL_PAGE_SIZE)
        {
for_page_remain: ;
            uintptr_t v_base = dynamic_va_base + i;
            if (i < exe_load->phdr[index].p_filesz)
            {
                // contious
                buffer_begin  = list_entry(load_node, page_node_t, list)->kva_begin;  
                load_node = load_node->next;
                 
                uint64_t page_top = ROUND(v_base, 0x1000);
                uint64_t page_remain = (uint64_t)page_top - (uint64_t)(v_base); 
                // printk("vaddr: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                //         exe_load->phdr[index].p_vaddr + i, page_top, page_remain);
                unsigned char *bytes_of_page;
                // load
                if (page_remain == 0) 
                {
                    if (!(exe_load->phdr[index].p_flags & PF_W) && !exe_load->dynamic)
                    {
                        alloc_page_point_phyc(v_base, pgdir, buffer_begin, MAP_USER);
                        bytes_of_page = (uchar *)buffer_begin;
                    } else 
                    {
                        // this will never happen for the first phdr
                        // else alloc and copy
                        bytes_of_page = alloc_page_helper(v_base, pgdir, MAP_USER);
                        kmemcpy(
                            bytes_of_page,
                            buffer_begin,
                            MIN(exe_load->phdr[index].p_filesz - i, NORMAL_PAGE_SIZE));
                    }
                } else 
                {
                    // if not align, we can make sure it will be the data phdr
                    bytes_of_page = alloc_page_helper(v_base, pgdir, MAP_USER);
                    // memcpy
                    kmemcpy(
                        bytes_of_page,
                        buffer_begin,
                        MIN(exe_load->phdr[index].p_filesz - i, page_remain)
                        );                    
                    if (exe_load->phdr[index].p_filesz - i <  page_remain) {
                        for (int j =
                            exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                            j < NORMAL_PAGE_SIZE; 
                            ++j) 
                        {
                            bytes_of_page[j] = 0;
                        }
                        continue;
                    } else 
                    {
                        i += page_remain;
                        goto for_page_remain;  
                    }                    
                }
                if (exe_load->phdr[index].p_filesz - i < NORMAL_PAGE_SIZE) {
                    for (int j =
                                exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                            j < NORMAL_PAGE_SIZE; ++j) {
                        bytes_of_page[j] = 0;
                    }
                }                
            } else 
            {
                // we can make sure it will be the data for .bss
                long *bytes_of_page =
                    (long *)alloc_page_helper(
                                  (uintptr_t)(v_base), 
                                                pgdir,
                                            MAP_USER);
                uint64_t clear_begin = (uint64_t)bytes_of_page & (uint64_t)0x0fff;
                for (int j = clear_begin;
                        j < NORMAL_PAGE_SIZE / sizeof(long);
                        ++j) {
                    bytes_of_page[j] = 0;
                }                
            }
        }
    }

    return exe_load->elf.entry + DYNAMIC_VADDR_PFFSET;    

}