#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>
#include <sbi.h>
#include <sbi_rust.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)
#define PTE_NUM (1lu << 9)
#define SATP_PPN_MASK ((1lu << SATP_ASID_SHIFT) - 1)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("fence\nfence.i\nsfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_cache_all(void)
{
    __asm__ __volatile__ ("fence\nfence.i" ::: "memory");
}

static inline void local_flush_icache_all(void)
{
    __asm__ __volatile__ ("fence.i" ::: "memory");
}

static inline void local_flush_dcache_all(void)
{
    __asm__ __volatile__ ("fence" ::: "memory");
}


static inline void flush_icache_all(void)
{
    local_flush_icache_all();
    sbi_remote_fence_i(NULL);
}

static inline void flush_tlb_all(void)
{
    local_flush_tlb_all();
    sbi_remote_sfence_vma(NULL, 0, -1);
}
static inline void flush_tlb_page_all(unsigned long addr)
{
    local_flush_tlb_page(addr);
    sbi_remote_sfence_vma(NULL, 0, -1);
}

static inline int is_kva(uint64_t addr)
{
    return (addr & (uint64_t)(~0) << 38) == (uint64_t)(~0) << 38;
}

static inline uintptr_t kva2pa(uintptr_t kva)
{
    // TODO:
    /**
     * get the pa from kva 
     */
    /* mask == 0x0000000011111111 */
    uint64_t mask = (uint64_t)(~0) >> 32;

    return kva & mask ;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    // TODO:
    /**
     * get pa from kva
     */
    /* mask == 0xffffffc00000000 */
    uint64_t mask = (uint64_t)(~0) << 38;
    return (pa & (0xffffffff)) | mask;
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("csrw satp, %0" : : "rK"(__v) : "memory");
}

static inline uint64_t get_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    return __v;
}

static inline uint64_t get_pgdir()
{
    unsigned long __v;
    // return __v;
    __asm__ __volatile__("csrr %0, satp" : : "rK"(__v) : "memory");
    return pa2kva((__v & SATP_PPN_MASK) << NORMAL_PAGE_SHIFT);
}

#ifndef k210
    #define START_ENTRYPOINT 0xffffffc080200000lu
#else
    #define START_ENTRYPOINT 0xffffffc080000000lu

#endif

#ifndef k210
    #define KERNEL_ENTRYPOINT 0xffffffc080220000lu
    #define KERNEL_END 0xffffffc080a20000lu
#else
    #define KERNEL_ENTRYPOINT 0xffffffc080040000lu
    #define KERNEL_END 0xffffffc080800000lu
#endif

/* level1 pgdir */
#ifndef k210
    #define PGDIR_PA ((KERNEL_ENTRYPOINT & 0xfffffffflu) - 0x18000)
#else
    #define PGDIR_PA ((KERNEL_ENTRYPOINT & 0xfffffffflu) - 0x18000) 
#endif


#define BOOT_KERNEL (START_ENTRYPOINT & 0xfffffffflu)
#define BOOT_KERNEL_END (KERNEL_END & 0xfffffffflu)

// for qemu disk
#define QEMU_DISK_OFFSET (START_ENTRYPOINT + (64*1024*1024))
#define QEMU_DISK_END (QEMU_DISK_OFFSET + (64*1024*1024))
#define SWAP_QEMU_DISK (QEMU_DISK_OFFSET + (38*1024*1024))

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */
#define _PAGE_SD   (1 << 9)     /* Clock SD flag */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t PTE;



static inline uint64_t get_pa(PTE entry)
{
    // TODO:
    /**
     * get pa from PTE
     */
    /* mask == 0031111111111111 */
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    /* ppn * 4096(0x1000) */
    return ((entry & mask) >> _PAGE_PFN_SHIFT) << 12;
}

/* Get/Set page frame number of the `entry` */
static inline uint64_t get_pfn(PTE entry)
{
    // TODO:
    /**
     * get pfn
     */
    /*mask == 00c1111111111111*/
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    return (entry & mask) >> _PAGE_PFN_SHIFT; 
}

static inline uintptr_t get_pa_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * get kva from page
     */
    if (va & 0xffffffc000000000) {
        return kva2pa(va);
    }
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    /* first null */
    if (page_base[vpn[2]] == NULL)
        return NULL;    
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    /* second null */
    if (second_page[vpn[1]] == NULL)
        return NULL;    
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    /* if null */
    if (third_page[vpn[0]] == NULL)
        return NULL;    
    return (get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << NORMAL_PAGE_SHIFT));
}



static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * get kva from page
     */
    if (va & 0xffffffc000000000) {
        return va;
    }
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    /* first null */
    if (page_base[vpn[2]] == NULL)
        return NULL;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    /* second null */
    if (second_page[vpn[1]] == NULL)
        return NULL;    
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    /* if null */
    if (third_page[vpn[0]] == NULL)
        return NULL;    
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << NORMAL_PAGE_SHIFT)));
}

static inline uintptr_t get_pfn_of(uintptr_t va, uintptr_t pgdir_va){
    // TODO:
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
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));    
}

static inline PTE * get_PTE_of(uintptr_t va, uintptr_t pgdir_va){
    // TODO:
    /**
     * get the PTE of the va
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
    
    if (third_page[vpn[0]] == NULL || (third_page[vpn[0]] & _PAGE_PRESENT) == 0)
        return NULL;    
    return &third_page[vpn[0]];    
}

static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    // TODO:
    /* set pfn
     */
    *entry &= ((uint64_t)(~0) >> 54);
    *entry |= (pfn << _PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry)
{
    // TODO:
    /**
     * get flag
     */

    uint64_t mask = (uint64_t)(~0) >> 54;
    return entry & mask;
}

static inline void set_attribute(PTE *entry, uint64_t bits)
{
    // TODO:
    /**
     * set flag
     */
    *entry &= ((uint64_t)(~0) << 10);
    *entry |= bits;
}

/* check flag  */
static inline uint32_t check_attribute(PTE *entry, uint64_t bits)
{
    /**
     * check flag
     */
    return (*entry & bits) != 0;
}

/* check flag  */
static inline void clear_attribute(PTE *entry, uint64_t bits)
{
    /**
     * check flag
     */
    *entry &= ~(bits);
}

/* check flag  */
static inline void add_attribute(PTE *entry, uint64_t bits)
{
    /**
     * check flag
     */
    *entry |= bits;
}

static inline void clear_page(uintptr_t page_addr){
    for(uint64_t i = 0; i < 0x1000; i+=8){
        *((uint64_t *)(page_addr+i)) = 0;
    }    
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    // TODO:
    /**
     * clear page
     */
    for(uint64_t i = 0; i < 0x1000; i+=8){
        *((uint64_t *)(pgdir_addr+i)) = 0;
    }
}

#endif  // PGTABLE_H
