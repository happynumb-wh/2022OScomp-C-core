#include <os/string.h>
int kstrlen(const char *src)
{
    int i;
    for (i = 0; src[i] != '\0'; i++) {
    }
    return i;
}

void kmemcpy(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

void kmemset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *dst = (uint8_t *)dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}

void kbzero(void *dest, uint32_t len) { kmemset(dest, 0, len); }

void kmemmove(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    uint8_t temp[len];
    int32_t i;
    for (i = 0; len != 0; len--) {
        temp[i] = *src++;
    }
    for (; len != 0; len--) {
        *dest++ = temp[i];
    }
}

int kstrcmp(const char *str1, const char *str2)
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

char *kstrcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

char *kstrcat(char *dest, const char *src)
{
    char *tmp = dest;

    while (*dest != '\0') { dest++; }
    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

int katoi(char *s, uint32_t mode)
{
    int sum = 0;
    int base = mode;
    for(int i = 0; i < kstrlen(s); i++){
        sum *= mode;
        sum += s[i] - '0';
    }   
    return sum;
}

long int katol(const char* str)
{
    int base = 10;
    if ((str[0] == '0' && str[1] == 'x') ||
        (str[0] == '0' && str[1] == 'X')) {
        base = 16;
        str += 2;
    }
    long ret = 0;
    while (*str != '\0') {
        if ('0' <= *str && *str <= '9') {
            ret = ret * base + (*str - '0');
        } else if (base == 16) {
            if ('a' <= *str && *str <= 'f'){
                ret = ret * base + (*str - 'a' + 10);
            } else if ('A' <= *str && *str <= 'F') {
                ret = ret * base + (*str - 'A' + 10);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        ++str;
    }
    return ret;
}

int kmemcmp(const void *ptr1, const void *ptr2, size_t num)
{
    for (int i = 0; i < num; ++i) {
        if (((char*)ptr1)[i] != ((char*)ptr2)[i]) {
            return ((char*)ptr1)[i] - ((char*)ptr2)[i];
        }
    }
    return 0;
}