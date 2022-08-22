#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <sys/syscall.h>

#define CLOCKS_PER_SEC (sys_get_timebase())

typedef uint64_t clock_t;

clock_t clock();

typedef struct
{
    clock_t sec;  // 自 Unix 纪元起的秒数
    clock_t usec; // 微秒数
} TimeVal;


#endif /* TIME_H */
