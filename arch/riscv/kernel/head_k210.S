#include <asm.h>
#include <csr.h>

.section ".entry_function","ax"
ENTRY(_start)
  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  fence
  fence.i
  /* j loop */
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
   
  li t0, SR_FS
  csrc sstatus, t0

  bnez a0, set_for_second

  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sw zero, (a3)
  add a3, a3, 4
  blt a3, a4, clear_bss
clear_bss_done:
  li sp, BOOT_STACK1  
  call call_boot_kernel

set_for_second:

  li sp, BOOT_STACK2
  call call_boot_kernel

loop:
  wfi
  j loop

END(_start)

ENTRY(set_kernel_id)
  ret
END(set_kernel_id)

ENTRY(call_boot_kernel)
  call boot_kernel
END(call_boot_kernel)