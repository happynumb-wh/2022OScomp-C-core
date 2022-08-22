#include <os/string.h>

size_t strlen(const char *src)
{
    int i;
    for (i = 0; src[i] != '\0'; i++) {
    }
    return i;
}

void* memcpy(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

void* memset(void *dest, int val, size_t len)
{
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

void bzero(void *dest, uint32_t len) { memset(dest, 0, len); }

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}


char *strcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char *strcat(char *dest, const char *src)
{
    char *tmp = dest;

    while (*dest != '\0') { dest++; }
    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char *strtok(char *substr, char *str, const char delim)
{
    int len = strlen(str);
    if (len == 0)
        return NULL;
    for (int i = 0; i < len; i++){
        if (str[i] != delim)
            substr[i] = str[i];
        else{
            substr[i] = 0;
            return &str[i + 1];
        }
    }
    return NULL;
}
int atoi(char *s, uint32_t mode)
{
    int sum = 0;
    int base = mode;
    for(int i = 0; i < strlen(s); i++){
        sum *= mode;
        sum += s[i] - '0';
    }   
    return sum;
}
