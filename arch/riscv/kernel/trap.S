#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
//  la s0, exception_handler_entry
//  csrw CSR_STVEC, s0
//  csrr t0, CSR_MHARTID
//  bnez t0, cond
//  csrw sip, zero
//cond:
//  li s0, SIE_STIE
//  csrw sie, s0
//  jr ra
// mhartid
  la t0, exception_handler_entry 
  csrw CSR_STVEC, t0
  li t0, SIE_STIE
  csrw CSR_SIE  , t0
  # li t0, SIE_SEIE
  # csrs CSR_SIE, t0
  # li t0, SIE_SSIE
  # csrs CSR_SIE, t0
  jr ra
ENDPROC(setup_exception)
