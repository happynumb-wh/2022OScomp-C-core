SHELL := /bin/bash
HOST_CC = gcc
CROSS_PREFIX = riscv64-unknown-elf-
CC = ${CROSS_PREFIX}gcc
AR = ${CROSS_PREFIX}ar
OBJCOPY = ${CROSS_PREFIX}objcopy
OBJDUMP = ${CROSS_PREFIX}objdump
CFLAGS = -O0 -w -nostdlib -T $(SRC_KLINKER) -Wall -mcmodel=medany -Iinclude \
		 -nostdinc -g -fvar-tracking -ffreestanding
USER_CFLAGS = -O0 -w -nostdlib -T $(SRC_ULINKER) -Wall -mcmodel=medany -Itest -Itiny_libc/include \
		 -Iinclude -nostdinc -fvar-tracking
USER_LDFLAGS = -L./ -ltiny_libc

# k210
START_ENTRYPOINT = 0x80020000
KERNEL_ENTRYPOINT = 0xffffffc080040000

# platform : k210 or qemu
TARGET = k210

ifeq ($(TARGET), qemu)
	START_ENTRYPOINT = 0x80200000
	KERNEL_ENTRYPOINT = 0xffffffc080220000
	CFLAGS += -D QEMU
	USER_CFLAGS += -D QEMU -g
else
	CFLAGS += -D k210
	USER_CFLAGS += -D k210
endif


# k210 port
K210_SERIALPORT	= /dev/ttyUSB0

ARCH        = riscv
ARCH_DIR    = ./arch/$(ARCH)
CFLAGS       += -Iarch/$(ARCH)/include
CFLAGS       += -Idrivers/screen
CFLAGS       += -Idrivers/sdcard/include
USER_CFLAGS  += -Iarch/$(ARCH)/include
USER_LDFLAGS += $(ARCH_DIR)/crt0.o

SRC_HEAD_QEMU = $(ARCH_DIR)/kernel/head_k210.S \
			    $(ARCH_DIR)/kernel/boot.c \
			    ./libs/string.c \
				$(ARCH_DIR)/kernel/sysctl.c
SRC_HEAD_K210 = $(ARCH_DIR)/kernel/head_k210.S \
			    $(ARCH_DIR)/kernel/boot.c \
			    ./libs/string.c \
			    $(ARCH_DIR)/kernel/sysctl.c

SRC_ARCH	= $(ARCH_DIR)/kernel/trap.S  \
			  $(ARCH_DIR)/kernel/entry.S \
			  $(ARCH_DIR)/kernel/start.S \
			  $(ARCH_DIR)/sbi/common.c \
			  $(ARCH_DIR)/kernel/smp.S
			  
SRC_DRIVER	= ./drivers/screen/screen.c \
			  ./drivers/sdcard/*.c
SRC_INIT 	= ./init/main.c
SRC_INT		= ./kernel/irq/irq.c
SRC_LOCK	= ./kernel/locking/lock.c\
			  ./kernel/locking/futex.c

SRC_SCHED	= ./kernel/sched/sched.c \
			  ./kernel/sched/init.c \
			  ./kernel/sched/id.c \
			  ./kernel/sched/signal.c \
			  ./kernel/sched/Signal.S \
			  ./kernel/sched/restore.S

SRC_TIME    = ./kernel/time/time.c \
			  ./kernel/time/sleep.c \
			  ./kernel/time/utlis.c

SRC_MM	    = ./kernel/mm/mm.c \
			  ./kernel/mm/recycle.c \
			  ./kernel/mm/mmirq.c \
			  ./kernel/mm/mmap.c

SRC_SMP     = ./kernel/smp/spinlock.c \
			  ./kernel/smp/smp.c \
			  ./kernel/smp/sleeplock.c
SRC_SYSCALL	= ./kernel/syscall/syscall.c
SRC_SYSTEM  = ./kernel/system/uname.c \
			  ./kernel/system/system.c \
			  ./kernel/system/io.c \
			  ./kernel/system/ring_buffer.c \
			  ./kernel/socket/socket.c 
SRC_ELF     = ./kernel/elf/elf.c \
			  ./kernel/elf/lazy_elf.c \
			  ./kernel/elf/fast_elf.c \
			  ./kernel/elf/dynamic.c
			  
SRC_UTILS   = ./kernel/utils/utils.c

SRC_LIBS	= ./libs/string.c \
			  ./libs/printk.c

SRC_LIBC	= ./tiny_libc/printf.c \
			  ./tiny_libc/string.c \
			  ./tiny_libc/syscall.c \
			  ./tiny_libc/invoke_syscall.S \
		      ./tiny_libc/time.c \
			  ./tiny_libc/rand.c \
			  ./tiny_libc/atol.c

SRC_LIBC_ASM = $(filter %.S %.s,$(SRC_LIBC))
SRC_LIBC_C	= $(filter %.c,$(SRC_LIBC))

SRC_USER	= ./test/shell.c
			  

SRC_UELF    = ./test/shell.elf \
			  ./test/etc/localtime \
			  ./test/etc/passwd \
			  ./test/etc/adjtime \
			  ./test/etc/group \
			  ./test/proc/mounts \
			  ./test/proc/meminfo \
			  ./test/bin/ls

			  
SRC_FAT32   = ./kernel/fat32/fat32.c \
			  ./kernel/fat32/pipe.c  \
			  ./kernel/fat32/mount.c \
			  ./kernel/fat32/mmap.c \
			  ./kernel/fat32/poll.c

SRC_UTILS   = ./kernel/utils/utils.c \
			  ./kernel/utils/special_ctx.c \
			  ./kernel/fat32/file.c

SRC_SWAP    = ./kernel/swap/swap.c

SRC_MAIN	= ${SRC_ARCH} \
			  ${SRC_INIT} \
			  ${SRC_INT} \
			  ${SRC_DRIVER} \
			  ${SRC_LOCK} \
			  ${SRC_SCHED} \
		      ${SRC_TIME} \
			  ${SRC_MM} \
			  ${SRC_SMP} \
			  ${SRC_SYSCALL} \
			  ${SRC_LIBS} \
			  ${SRC_SYSTEM} \
			  ${SRC_ELF} \
			  ${SRC_UTILS} \
			  ${SRC_FAT32} \
			  ${SRC_SWAP}
			
# make tools
SRC_IMAGE	    = ./tools/createimage.c
SRC_ELF2CHAR	= ./tools/elf2char.c
SRC_GENMAP	    = ./tools/generateMapping.c
SRC_GETMEMTOP	= ./tools/getmemtop.c

# official tools to run k210
SRC_BURNER	= ./tools/kflash.py
# linker 
SRC_KLINKER = ./linker/riscv.lds
SRC_ULINKER = ./linker/user_riscv.lds


.PHONY:all main clean

all: elf elf2char image asm

k210: elf elf2char image asm
	
qemu: elf elf2char qemu_head asm 

# k210 head
k210_head: $(SRC_HEAD_K210) $(SRC_KLINKER) createimage
	${CC} ${CFLAGS} -o ./target/k210_head $(SRC_HEAD_K210) -Ttext=${START_ENTRYPOINT} -Wall 
#${OBJCOPY} ./target/k210_head --strip-all -O binary ./target/k210_head.bin
	./createimage ./target/k210_head ./target/k210_head.bin
	${OBJDUMP} -d ./target/k210_head > ./debug/k210_head.txt 
# qemu head
qemu_head: $(SRC_HEAD_QEMU) $(SRC_KLINKER) main createimage
	${CC} ${CFLAGS} -o ./target/qemu_head $(SRC_HEAD_QEMU) -Ttext=${START_ENTRYPOINT} -Wall
#${OBJCOPY} ./target/qemu_head --strip-all -O binary ./target/qemu_head.bin
	./createimage ./target/qemu_head ./target/qemu_head.bin
	dd if=./target/kernel.bin of=./target/qemu_head.bin bs=128K seek=1
	${OBJDUMP} -d ./target/qemu_head > ./debug/qemu_head.txt
	cp ./target/qemu_head.bin ./kernel-qemu
	cp ./bootloader/rustsbi-qemu ./sbi-qemu


# user 
user: $(SRC_UELF) elf2char generateMapping elf
	echo "" > user_programs.c
	echo "" > user_programs.h
	@for prog in $(SRC_UELF); \
	do \
		./elf2char --header-only $$prog >> user_programs.h; \
	done
	@for prog in $(SRC_UELF); \
	do \
		./elf2char $$prog >> user_programs.c; \
	done
	./generateMapping user_programs
# for prog in $(SRC_UELF); \
# do \
# 	${OBJDUMP} -d $$prog >> $${prog/.elf/.txt}; \
# done
	${OBJDUMP} -d ./test/shell.elf > debug/shell.txt 
	mv user_programs.h include/
	mv user_programs.c kernel/

# dynamic link libraries
libtiny_libc.a: $(SRC_LIBC_C) $(SRC_LIBC_ASM) $(SRC_ULINKER)
	@for libobj in $(SRC_LIBC_C); \
	do \
		${CC} ${USER_CFLAGS} -c $$libobj -o $${libobj/.c/.o}; \
	done
	@for libobj in $(SRC_LIBC_ASM); \
	do \
		${CC} ${USER_CFLAGS} -c $$libobj -o $${libobj/.S/.o}; \
	done
	${AR} rcs libtiny_libc.a $(patsubst %.c, %.o, $(patsubst %.S, %.o,$(SRC_LIBC)))

$(ARCH_DIR)/crt0.o: $(ARCH_DIR)/crt0.S
	${CC} ${USER_CFLAGS} -c $(ARCH_DIR)/crt0.S -o $(ARCH_DIR)/crt0.o

elf: $(SRC_ULINKER) libtiny_libc.a $(ARCH_DIR)/crt0.o
	@for user in ${SRC_USER}; \
	do \
		${CC} -w ${USER_CFLAGS} $$user ${SRC_LIBC} $(ARCH_DIR)/crt0.S -o $${user/.c/.elf}; \
	done

main: $(SRC_MAIN) user $(SRC_KLINKER) createimage
	${CC} ${CFLAGS} -o main $(SRC_MAIN)  ./kernel/user_programs.c -Ttext=${KERNEL_ENTRYPOINT}
# ${OBJCOPY} ./main --strip-all -O binary ./target/kernel.bin
	./createimage ./main ./target/kernel.bin
# tools
elf2char: $(SRC_ELF2CHAR)
	${HOST_CC} ${SRC_ELF2CHAR} -o elf2char -ggdb -Wall
generateMapping: $(SRC_GENMAP)
	${HOST_CC} ${SRC_GENMAP} -o generateMapping -ggdb -Wall
getmemtop: $(SRC_GETMEMTOP)
	${HOST_CC} ${SRC_GETMEMTOP} -o getmemtop -ggdb -Wall
createimage: ${SRC_IMAGE}
	${HOST_CC} ${SRC_IMAGE} -o createimage -ggdb -Wall

image: k210_head main getmemtop createimage
# ${OBJCOPY} ./bootloader/rustsbi-k210 --strip-all -O binary ./os.bin
	./createimage ./bootloader/rustsbi-k210 ./os.bin
	dd if=./target/k210_head.bin of=./os.bin bs=128K seek=1
	dd if=./target/kernel.bin of=./os.bin bs=128K seek=2
# ./getmemtop main 
# ./getmemtop ./target/k210_head
run:
#./run_qemu.sh
	sudo python3 ./tools/kflash.py -p ${K210_SERIALPORT} -b 1500000 os.bin
	sudo python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct ${K210_SERIALPORT} 115200
clean:
	rm -rf main libtiny_libc.a os.bin ./target/*
	rm include/user_programs.h kernel/user_programs.c
	rm ./debug/*.txt
	find . -name "*.o" -exec rm {} \;

asm:
	${OBJDUMP} -d main > ./debug/kernel.txt
	
	
