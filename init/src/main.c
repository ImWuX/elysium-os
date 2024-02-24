#include "syscall.h"

int main(int argc, char **vargs) {
    syscall1(1, '#');
    for(;;);

    return 0;
}