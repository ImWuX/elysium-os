#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <elysium/syscall.h>

static int debug_print(const char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
    char *str = malloc(512);
	int ret = vsnprintf(str, 512, fmt, list);
	while(*str != '\0') syscall1(1, *str++);
    va_end(list);
	return ret;
}

int main(int argc, char **vargs) {
    debug_print("%s\n", vargs[0]);

    struct utsname name;
    int r = uname(&name);
    if(r != 0) {
        debug_print("Uname error (%s)\n", strerror(errno));
        return 1;
    }

    debug_print("%s (%s %s)\n", name.sysname, name.nodename, name.version);

    return 0;
}