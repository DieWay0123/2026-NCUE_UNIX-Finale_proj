#include "../libdiag/diag_common.h"

#include <stdio.h>
#include <string.h>

static void print_help(void)
{
    puts("Usage: bbnetmon --list [--state STATE] [--no-header]");
    puts("       bbnetmon --summary");
    puts("");
    puts("TCP connection state monitor.");
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    diag_print_error("bbnetmon", "not implemented yet", "prototype scaffold is ready");
    return 1;
}
