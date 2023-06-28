#include "common.h"
#include <stdio.h>
#include <memory/pmm.h>
#include <arch/types.h>

void common_init(draw_context_t *ctx) {
    draw_rect(ctx, 0, 0, ctx->width, ctx->height, draw_color(48, 199, 196));
    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n\n");
    printf("Welcome to Elysium OS\n");

    pmm_stats_t *stats = pmm_stats();
    printf("PMM Stats (in pages of %#lx):\n  >> Free: %lu\n  >> Wired: %lu\n  >> Anon: %lu\n  >> Backed: %lu\n", ARCH_PAGE_SIZE, stats->free_pages, stats->wired_pages, stats->anon_pages, stats->backed_pages);

}