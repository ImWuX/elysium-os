#include "string.h"

size_t strlen(const char *str) {
    size_t counter = 0;
    while(str[counter] != 0) ++counter;
    return counter;
}

int strcmp(const char *lhs, const char *rhs) {
    for(size_t i = 0; ; i++) {
        if(lhs[i] != rhs[i])
            return lhs[i] < rhs[i] ? -1 : 1;
        if(!lhs[i])
            return 0;
    }
}