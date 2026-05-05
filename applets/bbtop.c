#include "../libdiag/diag_common.h"

#include <stdio.h>
#include <string.h>

static void print_help(void)
{
    puts("Usage: bbtop --snapshot [--sort cpu|mem|pid] [--limit N]");
    puts("       bbtop --tree");
    puts("       bbtop --interval SEC");
    puts("");
    puts("Lightweight process resource analyzer.");
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    diag_print_error("bbtop", "not implemented yet", "prototype scaffold is ready");
    return 1;
}
