#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <time.h>
#include <elib.h>

#define ASSERT(ASSERTION) if(!(ASSERTION)) { printf("Assertion failed (%i) [%s]: \"%s\"\n", errno, strerror(errno), #ASSERTION); return 1; }

int main(int argc, char **vargs) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    printf("%s\n", vargs[0]);

    /* Uname */
    struct utsname name;
    ASSERT(uname(&name) == 0);
    printf("%s (%s %s)\n", name.sysname, name.nodename, name.version);

    /* Test reading kernel symbols */
    FILE *kernel_symbols = fopen("/modules/kernelsymbols.txt", "r");
    ASSERT(kernel_symbols != NULL);

    int read_count = 32;
    char *data = malloc(read_count);
    ASSERT(fread(data, 1, read_count, kernel_symbols) == (size_t) read_count)
    printf("some data from kernelsymbols.txt: %.*s\n", read_count, data);
    free(data);

    ASSERT(fclose(kernel_symbols) == 0);

    /* Stat */
    struct stat stat_data;
    ASSERT(stat("/modules/kernelsymbols.txt", &stat_data) == 0)
    printf(
        "kernelsymbols.txt stat\n  st_dev: %#lx\n  st_ino: %#lx\n  st_mode: %#x\n  st_nlink: %#lx\n  st_uid: %#x\n  st_gid: %#x\n  st_rdev: %#lx\n  st_size: %li\n  st_blksize: %li\n  st_blocks: %li\n",
        stat_data.st_dev,
        stat_data.st_ino,
        stat_data.st_mode,
        stat_data.st_nlink,
        stat_data.st_uid,
        stat_data.st_gid,
        stat_data.st_rdev,
        stat_data.st_size,
        stat_data.st_blksize,
        stat_data.st_blocks
    );

    /* Create file, write to it, append to it, read from it */
    FILE *hello_file = fopen("/tmp/hello", "w");
    ASSERT(hello_file != NULL);
    ASSERT(fwrite("hello world", 1, 11, hello_file) == 11);
    ASSERT(fclose(hello_file) == 0)

    hello_file = fopen("/tmp/hello", "a");
    ASSERT(hello_file != NULL);
    ASSERT(fwrite("xd", 1, 3, hello_file) == 3);
    ASSERT(fclose(hello_file) == 0);

    hello_file = fopen("/tmp/hello", "r");
    char *hello_data = malloc(14);
    ASSERT(fread(hello_data, 1, 14, hello_file) == 14);
    printf("hello data: %s\n", hello_data);
    free(hello_data);
    ASSERT(fclose(hello_file) == 0);

    /* Clock testing */
    struct timespec tp;
    ASSERT(clock_getres(CLOCK_REALTIME, &tp) == 0);
    printf("Realtime resolution: %lu, %lu\n", tp.tv_sec, tp.tv_nsec);

    ASSERT(clock_gettime(CLOCK_REALTIME, &tp) == 0);
    printf("Realtime: %lu, %lu\n", tp.tv_sec, tp.tv_nsec);

    /* Acquire framebuffer */
    elib_framebuffer_info_t fb_info;
    void *fb = elib_acquire_framebuffer(&fb_info);
    if(fb == NULL) {
        printf("Failed to acquire the framebuffer\n");
        return 1;
    }
    printf("Acquired framebuffer\n");

    printf("w: %lu, h: %lu, p: %lu\n", fb_info.width, fb_info.height, fb_info.pitch);
    for(uint64_t y = 0; y < fb_info.height; y++) {
        for(uint64_t x = 0; x < fb_info.width; x++) {
            *((uint32_t *) (fb + (y * fb_info.pitch) + (x * 4))) = ((255 << 16) | (255 << 8) | (0 << 0));
        }
    }

    ASSERT(clock_gettime(CLOCK_REALTIME, &tp) == 0);
    printf("Realtime: %lu, %lu\n", tp.tv_sec, tp.tv_nsec);

    return 0;
}