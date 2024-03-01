#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

int main(int argc, char **vargs) {
    printf("%s\n", vargs[0]);

    struct utsname name;
    int r = uname(&name);
    if(r != 0) {
        printf("Uname error (%s)\n", strerror(errno));
        return 1;
    }

    printf("%s (%s %s)\n", name.sysname, name.nodename, name.version);

    printf("----\n");

    return 0;
}