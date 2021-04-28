#ifndef __STRTOINT_H__
#define __STRTOINT_H__

#include <stdlib.h>

static inline int strtoint(const char *str, int *dest)
{
    char *endptr;
    int val = strtol(str,&endptr, 0);
    if (endptr) {
        if (*endptr=='\0') {
            *dest = val;
            return 0;
        }
    }
    return -1;
}

static inline int strtoint_octal(const char *str, int *dest)
{
    char *endptr;
    int val = strtol(str,&endptr, 8);
    if (endptr) {
        if (*endptr=='\0') {
            *dest = val;
            return 0;
        }
    }
    return -1;
}


#endif
