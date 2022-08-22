#ifndef SBI_RUST_H
#define SBI_RUST_H
#include <asm/sbidef.h>
#include <type.h>
#define SBI_SUCCESS 				0
#define SBI_ERR_FAILED 				-1
#define SBI_ERR_NOT_SUPPORTED 		-2
#define SBI_ERR_INVALID_PARAM		-3
#define SBI_ERR_DENIED 				-4
#define SBI_ERR_INVALID_ADDRESS		-5
#define SBI_ERR_ALREADY_AVAILABLE	-6
#define SBI_ERR_ALREADY_STARTED		-7
#define SBI_ERR_ALREADY_STOPPED		-8

struct sbiret {
	long error;
	long value;
};

/**
 * we will use ecall to use the SBI 
 * the a6 will be the extention
 * the a7 whill be thee func_id
 * the other will be param
 */

#define SBI_CALL(eid, fid, arg0, arg1, arg2, arg3) ({ \
	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0); \
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1); \
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2); \
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3); \
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid); \
	register uintptr_t a7 asm ("a7") = (uintptr_t)(eid); \
	asm volatile ("ecall" \
			: "+r" (a0), "+r" (a1) \
			: "r" (a2), "r" (a3), "r" (a6), "r" (a7) \
			: "memory"); \
	(struct sbiret){.error = (long)a0, .value = (long)a1}; \
})

#define SBI_CALL_0(eid, fid) \
	SBI_CALL(eid, fid, 0, 0, 0, 0)
#define SBI_CALL_1(eid, fid, arg0) \
	SBI_CALL(eid, fid, arg0, 0, 0, 0) 
#define SBI_CALL_2(eid, fid, arg0, arg1) \
	SBI_CALL(eid, fid, arg0, arg1, 0, 0) 
#define SBI_CALL_3(eid, fid, arg0, arg1, arg2) \
	SBI_CALL(eid, fid, arg0, arg1, arg2, 0) 
#define SBI_CALL_4(eid, fid, arg0, arg1, arg2, arg3) \
	SBI_CALL(eid, fid, arg0, arg1, arg2, arg3) 

// Time Extension 

#define TIME_EID 	0x54494d45
#define TIME_SET_TIMER 	0

static inline struct sbiret sbi_set_timer_k210(uint64_t stime_value) {
	return SBI_CALL_1(TIME_EID, TIME_SET_TIMER, stime_value);
}

// spi Extension 

#define IPI_EID 	0x735049
#define IPI_SEND_IPI	0

static inline struct sbiret sbi_send_ipi_k210(
	unsigned long hart_mask, 
	unsigned long hart_mask_base
) {
	return SBI_CALL_2(IPI_EID, IPI_SEND_IPI, hart_mask, hart_mask_base);
}

#endif