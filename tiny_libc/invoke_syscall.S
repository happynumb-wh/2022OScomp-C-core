#include <asm.h>
/* 
 * RISCV系统调用约定，将调用号放到a7寄存器中
 * extern long invoke_syscall(long, long, long, long);
 * a0 = 调用号
 * a1, a2, a3 = 参数
*/
ENTRY(invoke_syscall)
  /* TODO: */
  mv a7, a0
  mv a0, a1
  mv a1, a2
  mv a2, a3
  mv a3, a4
  mv a4, a5
  mv a5, a6
  ecall
  jr ra
ENDPROC(invoke_syscall)

