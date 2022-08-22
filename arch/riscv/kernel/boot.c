/* RISC-V kernel boot stage */
#include <sysctl.h>
#include <context.h>
#include <os/elf.h>
#include <memlayout.h>
#include <pgtable.h>
#include <sbi.h>
#include <sbi_rust.h>
#define KERNEL_STACK_ADDR 0xffffffcf00010000lu

int alloc_num = 0;
typedef void (*kernel_entry_t)(unsigned long, uintptr_t);

/* get the length */ 
void itoa(uint64_t num,char *str)
{
    while (num)
    {
        *str++ = num%10 + '0';
        num /= 10;
    }
    *str++ = '\n';*str = '\0';
}

/********* setup memory mapping ***********/
uintptr_t alloc_page()
{
    static uintptr_t pg_base = PGDIR_PA;
    pg_base += 0x1000;//4KB
    alloc_num++;
    return pg_base;
}

// using 2MB large page
void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    va &= VA_MASK;//va_mask == 7fffffffff
    uint64_t vpn2 = 
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    // printk("vpn2: %d, addr: %lx\n", vpn2, pa);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    //if no second-level page directory
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        // every second-level page need 4KB
        uintptr_t newpage = alloc_page();
        set_pfn(&pgdir[vpn2], newpage >> NORMAL_PAGE_SHIFT);
        
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
       
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    // no exception in the boot!
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                       _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
}

void enable_vm(uint64_t pgdir)
{
    // write satp to enable paging
    set_satp(SATP_MODE_SV39, 0, pgdir >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */

void setup_vm()
{
    clear_pgdir(PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    for (uint64_t kva = START_ENTRYPOINT;
        kva < KERNEL_END; kva += LARGE_PAGE_SIZE) {
        map_page(kva, kva2pa(kva), early_pgdir);
    } 
    // #define for qemu
    #ifndef k210
        for (uint64_t kva = QEMU_DISK_OFFSET;
            kva < QEMU_DISK_END; kva += LARGE_PAGE_SIZE) {
            map_page(kva, kva2pa(kva), early_pgdir);
        }     
    #endif
    // map boot address
    for (uint64_t pa = BOOT_KERNEL; pa < BOOT_KERNEL_END;
         pa += LARGE_PAGE_SIZE) {
        map_page(pa, pa, early_pgdir);
    }
}

uintptr_t directmap(uintptr_t kva, uintptr_t pgdir)
{
    // ignore pgdir
    return kva;
}


/* this is used for mapping kernel virtual address into user page table */
void share_pgdir(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    kmemcpy((char *)dest_pgdir, (char *)src_pgdir, 0x1000);
}

/* this is used for test the 8MB */
void test_large(){
    uintptr_t large = 0x80700000;
    *(uintptr_t*)large = 0x19491001;
    if(*(uintptr_t*)large == 0x19491001){
        sbi_console_putstr("> [INIT] Succeed to open the 8MB mem\n");
    }
    return;
}

void ioremap(uint64_t phys_addr, uint64_t size){
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    uint64_t page_num = (size + LARGE_PAGE_SIZE-1) / LARGE_PAGE_SIZE;
    for (int i = 0; i<page_num; i++){
        map_page(phys_addr, kva2pa(phys_addr), early_pgdir);
    } 
}


void total_ioremap(){
    #ifdef k210
        // ioremap
        ioremap(FPIOA, 0x1000);
        
        ioremap(UARTHS, NORMAL_PAGE_SIZE);

        ioremap(GPIOHS, 0x1000);
        ioremap(GPIO, 0x1000);
        ioremap(SPI_SLAVE, 0x1000);
        ioremap(SPI0, 0x1000);
        ioremap(SPI1, 0x1000);
        ioremap(SPI2, 0x1000);
        ioremap(FPIOA_BASE_ADDR, 0x1000);
    #else
        ioremap(UART0, NORMAL_PAGE_SIZE);
         // virtio mmio disk interface
        ioremap(VIRTIO0, NORMAL_PAGE_SIZE);       
    #endif
        ioremap(CLINT, 0x10000);

        ioremap(PLIC, 0x400000);
    sbi_console_putstr("> [INIT] Succeed all the ioremap\n");

}

kernel_entry_t start_kernel = NULL;

/*********** start here **************/
int boot_kernel(unsigned long mhartid, uintptr_t riscv_dtb)
{

    uint64_t core_id = mhartid;
    if (mhartid == 0) {//master kernel
    #ifdef k210
        // open and test the 8MB
        sysctl_pll_enable(SYSCTL_PLL1);
        sysctl_clock_enable(SYSCTL_CLOCK_PLL1);
        test_large();
    #endif    
        // the kernel
        setup_vm();        
        // map all io
        total_ioremap();
        // enable vm
        enable_vm(PGDIR_PA);
        start_kernel = (kernel_entry_t)KERNEL_ENTRYPOINT;
        // for (int i = 0; i < alloc_num; i++)
        // {
        //     /* code */
        //     sbi_console_putstr("hh ");
        // }
        // sbi_console_putchar('\n');
    } else {
        // enable vm
        enable_vm(PGDIR_PA);
    }
    set_kernel_id(core_id);
    start_kernel(mhartid, riscv_dtb);
    return 0;
}
