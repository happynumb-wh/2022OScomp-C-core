#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

extern void* memcpy(uint8_t *dest, const uint8_t *src, uint32_t len);
extern void* memset(void *dest, int val, size_t len);

extern int strcmp(const char *str1, const char *str2);
extern char *strcpy(char *dest, const char *src);
extern char *strcat(char *dest, const char *src);
extern size_t strlen(const char *src);
int atoi(char *s, uint32_t mode);
char *strtok(char *substr, char *str, const char delim);

#endif /* STRING_H */
