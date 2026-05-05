#include "diag_common.h"

#include <stdio.h>

void diag_print_error(const char *tool, const char *message, const char *detail)
{
    if (detail != NULL && detail[0] != '\0') {
        fprintf(stderr, "%s: %s: %s\n", tool, message, detail);
        return;
    }

    fprintf(stderr, "%s: %s\n", tool, message);
}
