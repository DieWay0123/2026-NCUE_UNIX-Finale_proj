#include "../libdiag/diag_common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BBFSCHECK_EXIT_GENERAL 1
#define BBFSCHECK_EXIT_INVALID_ARGUMENT 2
#define BBFSCHECK_EXIT_READ_FAILURE 3
#define DEFAULT_SMALL_FILE_SIZE 4096UL
#define WARNING_THRESHOLD 80.0
#define CRITICAL_THRESHOLD 90.0

static void print_help(void)
{
    puts("Usage: bbfscheck --summary PATH");
    puts("       bbfscheck --inode PATH");
    puts("       bbfscheck --scan PATH [--max-depth N] [--small-file BYTES]");
    puts("");
    puts("Filesystem health checking tool.");
    puts("");
    puts("Options:");
    puts("  --summary PATH          Show filesystem block usage");
    puts("  --inode PATH            Show filesystem inode usage");
    puts("  --scan PATH             Scan directory metadata");
    puts("  --max-depth N           Limit scan recursion depth");
    puts("  --small-file BYTES      Count files at or below this size");
    puts("  --help                  Show this help text");
}

static int parse_nonnegative_long(const char *value, long *result)
{
    char *endptr;
    long parsed;

    errno = 0;
    parsed = strtol(value, &endptr, 10);
    if (errno != 0 || endptr == value || *endptr != '\0' || parsed < 0) {
        return -1;
    }

    *result = parsed;
    return 0;
}

static unsigned long blocks_to_kb(const diag_fs_info *info, unsigned long blocks)
{
    unsigned long block_kb;

    block_kb = info->block_size / 1024;
    if (block_kb == 0) {
        block_kb = 1;
    }

    return blocks * block_kb;
}

static int print_summary(const char *path)
{
    diag_fs_info info;

    if (diag_fs_get_info(path, &info) != 0) {
        diag_print_error("bbfscheck", "failed to read filesystem summary", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    puts("PATH TOTAL_KB USED_KB FREE_KB USE% STATUS");
    printf("%s %lu %lu %lu %.1f %s\n",
           path,
           blocks_to_kb(&info, info.total_blocks),
           blocks_to_kb(&info, info.used_blocks),
           blocks_to_kb(&info, info.free_blocks),
           info.block_usage_percent,
           diag_health_status(WARNING_THRESHOLD, CRITICAL_THRESHOLD,
                              info.block_usage_percent));

    return 0;
}

static int print_inode(const char *path)
{
    diag_fs_info info;

    if (diag_fs_get_info(path, &info) != 0) {
        diag_print_error("bbfscheck", "failed to read inode summary", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    puts("PATH TOTAL_INODES USED_INODES FREE_INODES USE% STATUS");
    printf("%s %lu %lu %lu %.1f %s\n",
           path,
           info.total_inodes,
           info.used_inodes,
           info.free_inodes,
           info.inode_usage_percent,
           diag_health_status(WARNING_THRESHOLD, CRITICAL_THRESHOLD,
                              info.inode_usage_percent));

    return 0;
}

static int print_scan(const char *path, int max_depth, unsigned long small_file_size)
{
    diag_fs_scan_info info;
    int rc;

    rc = diag_fs_scan_path(path, max_depth, small_file_size, &info);
    if (rc != 0 && info.files == 0 && info.directories == 0 && info.symlinks == 0 &&
        info.other == 0 && info.skipped_special == 0) {
        diag_print_error("bbfscheck", "failed to scan path", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    puts("PATH FILES DIRS SYMLINKS OTHER SMALL_FILES TOTAL_BYTES ERRORS SKIPPED_SPECIAL STATUS");
    printf("%s %lu %lu %lu %lu %lu %llu %lu %lu %s\n",
           path,
           info.files,
           info.directories,
           info.symlinks,
           info.other,
           info.small_files,
           info.total_size,
           info.errors,
           info.skipped_special,
           info.errors == 0 ? "OK" : "WARNING");

    return info.errors == 0 ? 0 : BBFSCHECK_EXIT_READ_FAILURE;
}

int main(int argc, char **argv)
{
    int i;
    int mode_count;
    const char *summary_path;
    const char *inode_path;
    const char *scan_path;
    int max_depth;
    unsigned long small_file_size;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    summary_path = NULL;
    inode_path = NULL;
    scan_path = NULL;
    max_depth = -1;
    small_file_size = DEFAULT_SMALL_FILE_SIZE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--summary") == 0) {
            if (++i >= argc) {
                diag_print_error("bbfscheck", "missing path for --summary", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            summary_path = argv[i];
        } else if (strcmp(argv[i], "--inode") == 0) {
            if (++i >= argc) {
                diag_print_error("bbfscheck", "missing path for --inode", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            inode_path = argv[i];
        } else if (strcmp(argv[i], "--scan") == 0) {
            if (++i >= argc) {
                diag_print_error("bbfscheck", "missing path for --scan", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            scan_path = argv[i];
        } else if (strcmp(argv[i], "--max-depth") == 0) {
            long parsed;

            if (++i >= argc || parse_nonnegative_long(argv[i], &parsed) != 0) {
                diag_print_error("bbfscheck", "invalid value for --max-depth", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            max_depth = (int)parsed;
        } else if (strcmp(argv[i], "--small-file") == 0) {
            long parsed;

            if (++i >= argc || parse_nonnegative_long(argv[i], &parsed) != 0) {
                diag_print_error("bbfscheck", "invalid value for --small-file", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            small_file_size = (unsigned long)parsed;
        } else {
            diag_print_error("bbfscheck", "unknown option", argv[i]);
            return BBFSCHECK_EXIT_INVALID_ARGUMENT;
        }
    }

    mode_count = (summary_path != NULL) + (inode_path != NULL) + (scan_path != NULL);
    if (mode_count != 1) {
        diag_print_error("bbfscheck", "choose exactly one mode", "--summary, --inode, or --scan");
        return BBFSCHECK_EXIT_INVALID_ARGUMENT;
    }

    if (summary_path != NULL) {
        return print_summary(summary_path);
    }

    if (inode_path != NULL) {
        return print_inode(inode_path);
    }

    return print_scan(scan_path, max_depth, small_file_size);
}
