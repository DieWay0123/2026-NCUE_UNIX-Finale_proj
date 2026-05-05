#include "../libdiag/diag_common.h"

#include <stdio.h>
#include <string.h>

static void print_help(void)
{
    puts("Usage: bbfscheck --summary PATH");
    puts("       bbfscheck --inode PATH");
    puts("       bbfscheck --scan PATH [--max-depth N] [--small-file BYTES]");
    puts("");
    puts("Filesystem health checking tool.");
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    diag_print_error("bbfscheck", "not implemented yet", "prototype scaffold is ready");
    return 1;
}
