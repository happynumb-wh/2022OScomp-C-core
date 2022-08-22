#include <elf.h>
#include <stdio.h>
typedef unsigned char uchar;

int is_elf_format(Elf64_Ehdr *binary)
{

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)binary;
    if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
        ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr->e_ident[EI_MAG3] == ELFMAG3) {
        return 0;
    }

    return 1;
}

/**
 * @brief get the top of the kernel image to confirm the Heaptop
 */
int main(int argc, char * argv[])
{
    char * name;
    if (argc >= 2) {
        name = argv[1];
    } else {
        name = "main";
    }
    printf("file name: %s\n", name);
    FILE *fp;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;
    fp = fopen(name,"rb"); 
    if (fp == NULL) {
        printf("no this source: %s\n", name);
        return 0;
    }
    fseek(fp, 0, SEEK_SET);
    fread(&ehdr, sizeof(Elf64_Ehdr), 1, fp);
    if (is_elf_format(&ehdr) == 1) {
        printf("%s is not a elf file!\n", name);
        return 0;
    }
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;

    ptr_ph_table   = ehdr.e_phoff;
    ph_entry_count = ehdr.e_phnum;
    ph_entry_size  = ehdr.e_phentsize;   

    printf("ptr_ph_table: %ld\n", ptr_ph_table);
    printf("ph_entry_count: %d\n", ph_entry_count);
    printf("ph_entry_size: %d\n", ph_entry_size);
    int i = 1;
    while (ph_entry_count--)
    {
        fseek(fp, ptr_ph_table, SEEK_SET);
        fread(&phdr, sizeof(Elf64_Phdr), 1, fp);
        printf("the %d phdr\n", i);
        printf("    phdr.p_vaddr:  0x%lx\n", phdr.p_vaddr);
        printf("    phdr.p_memsz:  0x%lx\n", phdr.p_memsz);
        printf("    phdr.p_filesz: 0x%lx\n", phdr.p_filesz);
        printf("    the top:       0x%lx\n", phdr.p_vaddr + phdr.p_memsz);
        ptr_ph_table += ph_entry_size;
    }
    

}
