#include <asm.h>
#include <csr.h>


# ENTRY(get_current_cpu_id)
# #  mv a0, tp
#   sd a0, OFFSET_CORE_ID(tp)
#   jr ra
# ENDPROC(get_current_cpu_id)

ENTRY(get_current_running)
#  bnez tp, get_slave
#  ld a0, current_running_master
#  jr ra 
#
#get_slave:
#  ld a0, current_running_slave
  mv a0, tp
  jr ra
ENDPROC(get_current_running)

