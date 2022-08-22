#include <os/elf.h>
#include <os/sched.h>
#include <fs/fat32.h>
#include <utils/utils.h>
#include <assert.h>
// debug print message of ehdr
void debug_print_ehdr(Elf64_Ehdr ehdr)
{
    printk("=============ehdr===============\n");
    printk("ehdr.e_phoff: 0x%lx\n", ehdr.e_phoff);
    printk("ehdr.e_phnum: 0x%lx\n", ehdr.e_phnum);
    printk("ehdr.e_phentsize: 0x%lx\n", ehdr.e_phentsize);
    printk("ehdr->e_ident[0]: 0x%lx\n", ehdr.e_ident[0]);
    printk("ehdr->e_ident[1]: 0x%lx\n", ehdr.e_ident[1]);
    printk("ehdr->e_ident[2]: 0x%lx\n", ehdr.e_ident[2]);
    printk("ehdr->e_ident[3]: 0x%lx\n", ehdr.e_ident[3]);
    printk("=============end ehdr===============\n");
}

// debug print message of phdr
void debug_print_phdr(Elf64_Phdr phdr)
{
    printk("=============phdr===============\n");
    printk("phdr.p_offset: 0x%lx\n", phdr.p_offset);
    printk("phdr.p_vaddr: 0x%lx\n", phdr.p_vaddr);
    printk("phdr.p_paddr: 0x%lx\n", phdr.p_paddr);
    printk("phdr.p_filesz: 0x%lx\n", phdr.p_filesz);
    printk("phdr.p_memsz: 0x%lx\n", phdr.p_memsz);
    printk("=============end phdr===============\n");
}

/* prepare_page_for_kva should return a kernel virtual address */
uintptr_t load_elf(
    unsigned char elf_binary[], unsigned length, uintptr_t pgdir, uint64_t *file_length, void *initpcb ,
    uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir, uint64_t mode))
{
    pcb_t *underinitpcb = (pcb_t *)initpcb;
    // kmemcpy(&underinitpcb->elf, )
    underinitpcb->elf.edata = 0;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_binary;

    Elf64_Phdr *phdr = NULL;
    Elf64_Shdr *shdr = NULL;    
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    unsigned char *ptr_ph_table = NULL;
    unsigned char *ptr_sh_table = NULL;    
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    Elf64_Half sh_entry_count;
    Elf64_Half sh_entry_size;
    int i;
    // printk("entry: 0x%lx\n", ehdr->e_entry);
    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format(elf_binary)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = elf_binary + ehdr->e_phoff;
    ptr_sh_table   = elf_binary + ehdr->e_shoff;
    ph_entry_count = ehdr->e_phnum;
    ph_entry_size  = ehdr->e_phentsize;
    sh_entry_count = ehdr->e_shnum;
    sh_entry_size  = ehdr->e_shentsize;

    // save all useful message
    underinitpcb->elf.phoff = ehdr->e_phoff;
    underinitpcb->elf.phent = ehdr->e_phentsize;
    underinitpcb->elf.phnum = ehdr->e_phnum;
    underinitpcb->elf.entry = ehdr->e_entry;
    // printk("ehdr->e_phoff: 0x%lx\n", ehdr->e_phoff);
    // printk("ptr_ph_table: 0x%lx\n", ptr_ph_table);

    int is_first = 1; 
    // load elf
    // debug_print_ehdr(*ehdr);
    while (ph_entry_count--) {
        phdr = (Elf64_Phdr *)ptr_ph_table;

        *file_length += phdr->p_memsz;
        if (phdr->p_type == PT_LOAD) {
            // debug_print_phdr(*phdr);
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
page_remain_qemu: ;
                if (i < phdr->p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)prepare_page_for_va(
                            (uintptr_t)(phdr->p_vaddr + i), 
                                                    pgdir,
                                                MAP_USER);
                    // not 0x1000 page align
                    uint64_t page_top = ROUND(bytes_of_page, 0x1000);

                    uint64_t page_remain = (uint64_t)page_top - \
                                    (uint64_t)(bytes_of_page);   

                    // printk("vaddr: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                    //     (uint64_t)phdr->p_vaddr + i, page_top, page_remain);                
                    
                    if (page_remain == 0)                         
                        kmemcpy(
                            bytes_of_page,
                            elf_binary + phdr->p_offset + i,
                            MIN(phdr->p_filesz - i, NORMAL_PAGE_SIZE));
                    else 
                    {
                        
                        kmemcpy(
                            bytes_of_page,
                            elf_binary + phdr->p_offset + i,
                            MIN(phdr->p_filesz - i, page_remain));
                        // if less than page remain, point has ok, and we call add 0x1000
                        if (phdr->p_filesz - i <  page_remain) {
                            for (int j =
                                    (phdr->p_vaddr + phdr->p_filesz) % NORMAL_PAGE_SIZE;
                                j < NORMAL_PAGE_SIZE; ++j) {
                                bytes_of_page[j] = 0;
                            }
                            continue;
                        } else 
                        {
                            i += page_remain;
                            goto page_remain_qemu;  
                        }
                                                                               
                    }

                    // printk("bytes_of_page: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                    //     (uint64_t)bytes_of_page, page_top, page_remain);
                    if (phdr->p_filesz - i < NORMAL_PAGE_SIZE) {
                        for (int j =
                                 (phdr->p_vaddr + phdr->p_filesz) % NORMAL_PAGE_SIZE;
                             j < NORMAL_PAGE_SIZE; ++j) {
                            bytes_of_page[j] = 0;
                        }
                    }
                } else {
                    long *bytes_of_page =
                        (long *)prepare_page_for_va(
                            (uintptr_t)(phdr->p_vaddr + i), 
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
            underinitpcb->elf.edata =  phdr->p_vaddr + phdr->p_memsz;
            underinitpcb->edata = phdr->p_vaddr + phdr->p_memsz;
            if (is_first) { 
                // debug_print_phdr(*phdr);
                underinitpcb->elf.text_begin = phdr->p_vaddr; 
                is_first = 0;
                // Elf64_Phdr * test_phdr =  (Elf64_Phdr *)(phdr->p_offset + ehdr->e_phoff + elf_binary);
                // debug_print_phdr(*test_phdr);
            }
        }

        ptr_ph_table += ph_entry_size;
    }
    // if (kstrcmp(underinitpcb->name, "shell"))
    //    while(1);

    return ehdr->e_entry;
}

/**
 * @brief 从SD卡加载整个进程执行
 * @param fd 文件描述符
 * @param pgdir 页表
 * @param file_length 加载的长度
 * @param prepare_page_for_va alloc_pagr_helper
 * @return return the entry of the process
 */
uintptr_t fat32_load_elf(uint32_t fd, uintptr_t pgdir, uint64_t *file_length, void *initpcb ,
    uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir, uint64_t mode))
{
    pcb_t * current_running = get_current_running();
    pcb_t *underinitpcb = (pcb_t *)initpcb;

    fd_t *nfd = &current_running->pfd[get_fd_index(fd, current_running)];   
    assert(nfd != NULL);
    uchar *load_buff = kmalloc(PAGE_SIZE);
    Elf64_Ehdr ehdr;// = (Elf64_Ehdr *)kmalloc(sizeof(Elf64_Ehdr));
    fat32_lseek(fd, 0, SEEK_SET);
    fat32_read(fd, &ehdr, sizeof(Elf64_Ehdr));
    // printk("entry: %lx\n", ehdr.e_entry);
    // debug_print_ehdr(ehdr);
    Elf64_Phdr phdr;// = (Elf64_Phdr *)kmalloc(sizeof(Elf64_Phdr));//NULL;
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;
    // check whether `binary` is a ELF file.
    if (nfd->length < 4 || !is_elf_format(&ehdr)) {
        return 0;  // return NULL when error!
    }
    ptr_ph_table   = ehdr.e_phoff;
    ph_entry_count = ehdr.e_phnum;
    ph_entry_size  = ehdr.e_phentsize;

    // save all useful message
    underinitpcb->elf.phoff = ehdr.e_phoff;
    underinitpcb->elf.phent = ehdr.e_phentsize;
    underinitpcb->elf.phnum = ehdr.e_phnum;
    underinitpcb->elf.entry = ehdr.e_entry;

    int is_first = 1;
    // load the elf
    while (ph_entry_count--) {
        fat32_lseek(fd, ptr_ph_table, SEEK_SET);
        fat32_read(fd, &phdr, sizeof(Elf64_Phdr));
        // debug_print_phdr(phdr);
        *file_length += phdr.p_memsz;
        if (phdr.p_type == PT_LOAD) {
            /* TODO: */
            // debug_print_phdr(phdr);
            for (i = 0; i < phdr.p_memsz; i += NORMAL_PAGE_SIZE) {
page_remain_k210: ;
                if (i < phdr.p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)prepare_page_for_va(
                               (uintptr_t)(phdr.p_vaddr + i), 
                                                       pgdir,
                                                   MAP_USER);
                   
                    // not 0x1000 page align
                    uint64_t page_top = ROUND(bytes_of_page, 0x1000);

                    uint64_t page_remain = (uint64_t)page_top - \
                                    (uint64_t)(bytes_of_page);   

                    // printk("vaddr: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                    //     phdr.p_vaddr + i, page_top, page_remain);

                    // debug_print_fd(nfd);
                    // for (int i = 0; i < num; i++)
                    // {
                    //     /* code */
                    //     printk("%d", load_buff[i]);
                    // }
                    // printk("\n");
                    // debug_print_fd(nfd);

                    fat32_lseek(fd, phdr.p_offset + i, SEEK_SET);
                    // debug_print_fd(nfd);
                    if (page_remain == 0) 
                    {
                        // debug_print_fd(nfd);
                        uint32_t num =  fat32_read(fd, load_buff, MIN(phdr.p_filesz - i, NORMAL_PAGE_SIZE));   
                        
                        // copy
                        kmemcpy(
                            bytes_of_page,
                            load_buff,
                            MIN(phdr.p_filesz - i, NORMAL_PAGE_SIZE));                                           
                    } else 
                    {
                        // debug_print_fd(nfd);
                        uint32_t num =  fat32_read(fd, load_buff, MIN(phdr.p_filesz - i, page_remain));                           
                        // for no align page_size

                        kmemcpy(
                            bytes_of_page,
                            load_buff,
                            MIN(phdr.p_filesz - i, page_remain));
                        // if less than page remain, point has ok, and we call add 0x1000
                        if (phdr.p_filesz - i <  page_remain) {
                            for (int j =
                                phdr.p_filesz % NORMAL_PAGE_SIZE;
                                j < NORMAL_PAGE_SIZE; ++j) {
                                bytes_of_page[j] = 0;
                            }
                            continue;
                        } else 
                        {
                            i += page_remain;
                            goto page_remain_k210;  
                        }
                    }
                    
                    if (phdr.p_filesz - i < NORMAL_PAGE_SIZE) {
                        for (int j =
                                 phdr.p_filesz % NORMAL_PAGE_SIZE;
                             j < NORMAL_PAGE_SIZE; ++j) {
                            bytes_of_page[j] = 0;
                        }
                    }
                } else {
                    long *bytes_of_page =
                        (long *)prepare_page_for_va(
                        (uintptr_t)(phdr.p_vaddr + i), 
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
            // save edata
            underinitpcb->elf.edata =  phdr.p_vaddr + phdr.p_memsz;
            underinitpcb->edata = phdr.p_vaddr + phdr.p_memsz;
            if (is_first) { 
                underinitpcb->elf.text_begin = phdr.p_vaddr; 
                is_first = 0; 
            }
        }
        ptr_ph_table += ph_entry_size;
    }
    fat32_close(fd);
    kfree(load_buff);
    // while(1);
    return ehdr.e_entry;
}
