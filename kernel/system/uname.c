#include <os/uname.h>
#include <os/string.h>
#include <stdio.h>

static utsname_t utsname = {
    .sysname = "Linux",
    .nodename = "debian",
    .release = "5.10.0-7-riscv64",
    .version = "#1 SMP Debian 5.10.40-1 (2022-05-31)",
    .machine = "riscv64",
    .domainname = "(none)"
};

/**
 * @brief return the system message
 * 
 */
uint8 do_uname(struct utsname *uts)
{
    // if (sizeof(uts) != sizeof(utsname_t))
    //     return -1;
    kstrcpy(uts->sysname, utsname.sysname); 
    kstrcpy(uts->nodename, utsname.nodename); 
    kstrcpy(uts->release, utsname.release);
    kstrcpy(uts->version, utsname.version);
    kstrcpy(uts->machine, utsname.machine);
    kstrcpy(uts->domainname, utsname.domainname);
    return 0;
}