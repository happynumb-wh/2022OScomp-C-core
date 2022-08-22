/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */

.section ".entry_function","ax"
ENTRY(_start)
  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  fence
  fence.i

  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero
  
  li t0, SR_FS
  csrc sstatus, t0

  /* Clear BSS for flat non-ELF images */
  bnez a0, set_secondary_stack
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sw zero, (a3)
  add a3, a3, 4
  blt a3, a4, clear_bss
clear_bss_done:
  ld sp, pid0_stack_m
  la tp, pid0_pcb_master
  la t0, pid0_pcb_master
  csrw CSR_SSCRATCH, t0  
  j end_set_stack

set_secondary_stack:
/* setup C environment */
  ld sp, pid0_stack_s
  la tp, pid0_pcb_slave
  la t0, pid0_pcb_slave
  csrw CSR_SSCRATCH, t0

end_set_stack:
# csrw CSR_SSCRATCH, x0

  call main

loop:
  wfi
  j loop

END(_start)
