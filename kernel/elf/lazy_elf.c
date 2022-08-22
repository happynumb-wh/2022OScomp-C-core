#include <os/elf.h>
#include <os/sched.h>
#include <fs/fat32.h>
#include <utils/utils.h>
#include <assert.h>


// lazy load_elf for k210
extern uintptr_t lazy_fat32_load_elf(uint32_t fd, uintptr_t pgdir, uint64_t *file_length, void *initpcb ,
    uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir, uint64_t mode))
{
    // TODO
}