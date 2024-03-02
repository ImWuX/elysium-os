#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/stat.h>

int main(int argc, char **vargs) {
    printf("%s\n", vargs[0]);

    struct utsname name;
    int r = uname(&name);
    if(r != 0) {
        printf("Uname error (%s)\n", strerror(errno));
        return 1;
    }

    printf("%s (%s %s)\n", name.sysname, name.nodename, name.version);

    FILE *kernel_symbols = fopen("/modules/KERNSYMBTXT", "r");
    if(kernel_symbols == NULL) {
        printf("Failed to open kernsymb.txt (%s)\n", strerror(errno));
        return 1;
    }

    int read_count = 32;
    char *data = malloc(read_count);
    size_t rc = fread(data, 1, read_count, kernel_symbols);
    if(rc != (size_t) read_count) {
        printf("Failed to read from kernsymb.txt (%s)\n", strerror(errno));
        return 1;
    }
    r = fclose(kernel_symbols);
    if(r != 0) {
        printf("Failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    printf("some data from kernsymb.txt: %.*s\n", read_count, data);
    free(data);

    struct stat stat_data;
    r = stat("/modules/KERNSYMBTXT", &stat_data);
    if(r != 0) {
        printf("Failed to stat file (%s)\n", strerror(errno));
        return 1;
    }
    printf("kernsymb.txt size: %li\n", stat_data.st_size);

    return 0;
}