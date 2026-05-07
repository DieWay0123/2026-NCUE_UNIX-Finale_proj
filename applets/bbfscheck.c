#include "../libdiag/diag_common.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BBFSCHECK_EXIT_GENERAL 1
#define BBFSCHECK_EXIT_INVALID_ARGUMENT 2
#define BBFSCHECK_EXIT_READ_FAILURE 3
#define BBFSCHECK_EXIT_CRITICAL 4

#define DEFAULT_SMALL_FILE_SIZE 4096UL
#define DEFAULT_BLOCK_SIZE 1024ULL
#define WARNING_THRESHOLD 80.0
#define CRITICAL_THRESHOLD 90.0
#define SMALL_FILE_RATIO_WARNING 70.0
#define SMALL_FILE_RATIO_CRITICAL 90.0

typedef enum {
    DISPLAY_BLOCKS,
    DISPLAY_HUMAN_1024,
    DISPLAY_HUMAN_1000
} display_mode;

typedef struct {
    display_mode mode;
    unsigned long long block_size;
} display_options;

static void print_help(void)
{
    puts("Usage: bbfscheck --summary PATH [OPTIONS]");
    puts("       bbfscheck --inode PATH");
    puts("       bbfscheck --scan PATH [--max-depth N] [--small-file BYTES] [OPTIONS]");
    puts("       bbfscheck --check PATH [--max-depth N] [--small-file BYTES] [OPTIONS]");
    puts("");
    puts("Filesystem health checking tool.");
    puts("");
    puts("Options:");
    puts("  --summary PATH          Show filesystem block usage");
    puts("  --inode PATH            Show filesystem inode usage");
    puts("  --scan PATH             Scan directory metadata");
    puts("  --check PATH            Show combined filesystem health report");
    puts("");
    puts("Scan options:");
    puts("  --max-depth N           Limit scan recursion depth");
    puts("  --small-file BYTES      Count files at or below this size");
    puts("");
    puts("Size display options:");
    puts("  -h                      Human-readable sizes using powers of 1024");
    puts("  -H                      Human-readable sizes using powers of 1000");
    puts("  -B SIZE                 Show summary sizes in SIZE-byte blocks");
    puts("  --block-size SIZE       Same as -B SIZE; supports K, M, G, T suffixes");
    puts("");
    puts("Other options:");
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

static int parse_size_bytes(const char *value, unsigned long long *result)
{
    char *endptr;
    unsigned long long parsed;
    unsigned long long multiplier;

    if(value == NULL || result == NULL) {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    parsed = strtoull(value, &endptr, 10); // e.g.: -b 4k => value=4, endptr=k
    if(errno != 0 || endptr == value){
        return -1;
    }

    multiplier = 1ULL;
    if(*endptr != '\0'){
        if (endptr[1] != '\0'){
            return -1;
        }

        switch(*endptr){
            case 'k':
            case 'K':
                multiplier = 1024ULL;
                break;
            case 'm':
            case 'M':
                multiplier = 1024ULL * 1024ULL;
                break;
            case 'g':
            case 'G':
                multiplier = 1024ULL * 1024ULL * 1024ULL;
                break;
            case 't':
            case 'T':
                multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
                break;
            default:
                return -1;
        }
    }

    if (parsed == 0 || parsed > ULLONG_MAX / multiplier) {
        return -1;
    }

    *result = parsed * multiplier;
    return 0;
}

// div + ceil
static unsigned long long ceil_div_ull(unsigned long long value, unsigned long long divisor)
{
    if (divisor == 0) {
        return 0;
    }

    return value / divisor + (value % divisor != 0);
}

static unsigned long long blocks_to_bytes(const diag_fs_info *info, unsigned long blocks)
{
    return (unsigned long long)blocks * (unsigned long long)info->block_size;
}

static void format_human_size(char *buffer, size_t buffer_size,
                              unsigned long long bytes, int base)
{
    const char *binary_units[] = {"B", "K", "M", "G", "T", "P", "E"};
    const char *decimal_units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    const char **units;
    double value;
    size_t unit_index;

    units = base == 1000 ? decimal_units : binary_units;
    value = (double)bytes;
    unit_index = 0;

    while (value >= (double)base && unit_index < 6) {
        value /= (double)base;
        unit_index++;
    }

    if (unit_index == 0) {
        snprintf(buffer, buffer_size, "%llu%s", bytes, units[unit_index]);
    } else if (value >= 10.0) {
        snprintf(buffer, buffer_size, "%.0f%s", value, units[unit_index]);
    } else {
        snprintf(buffer, buffer_size, "%.1f%s", value, units[unit_index]);
    }
}

static void format_fs_blocks(char *buffer, size_t buffer_size,
                             const diag_fs_info *info, unsigned long blocks,
                             const display_options *display)
{
    unsigned long long bytes;
    unsigned long long units;

    bytes = blocks_to_bytes(info, blocks);

    if (display->mode == DISPLAY_HUMAN_1024) {
        format_human_size(buffer, buffer_size, bytes, 1024);
        return;
    }

    if (display->mode == DISPLAY_HUMAN_1000) {
        format_human_size(buffer, buffer_size, bytes, 1000);
        return;
    }

    units = ceil_div_ull(bytes, display->block_size);
    snprintf(buffer, buffer_size, "%llu", units);
}

static void format_scan_size(char *buffer, size_t buffer_size,
                             unsigned long long bytes,
                             const display_options *display)
{
    if (display->mode == DISPLAY_HUMAN_1024) {
        format_human_size(buffer, buffer_size, bytes, 1024);
        return;
    }

    if (display->mode == DISPLAY_HUMAN_1000) {
        format_human_size(buffer, buffer_size, bytes, 1000);
        return;
    }

    snprintf(buffer, buffer_size, "%llu", bytes);
}

static void format_size_header(char *buffer, size_t buffer_size,
                               const display_options *display)
{
    unsigned long long block_size;

    if (display->mode == DISPLAY_HUMAN_1024 ||
        display->mode == DISPLAY_HUMAN_1000) {
        snprintf(buffer, buffer_size, "%s", "Size");
        return;
    }

    block_size = display->block_size;

    if (block_size >= 1024ULL * 1024ULL * 1024ULL * 1024ULL &&
        block_size % (1024ULL * 1024ULL * 1024ULL * 1024ULL) == 0) {
        snprintf(buffer, buffer_size, "%lluT-blocks",
                 block_size / (1024ULL * 1024ULL * 1024ULL * 1024ULL));
        return;
    }

    if (block_size >= 1024ULL * 1024ULL * 1024ULL &&
        block_size % (1024ULL * 1024ULL * 1024ULL) == 0) {
        snprintf(buffer, buffer_size, "%lluG-blocks",
                 block_size / (1024ULL * 1024ULL * 1024ULL));
        return;
    }

    if (block_size >= 1024ULL * 1024ULL &&
        block_size % (1024ULL * 1024ULL) == 0) {
        snprintf(buffer, buffer_size, "%lluM-blocks",
                 block_size / (1024ULL * 1024ULL));
        return;
    }

    if (block_size >= 1024ULL &&
        block_size % 1024ULL == 0) {
        snprintf(buffer, buffer_size, "%lluK-blocks", block_size / 1024ULL);
        return;
    }

    snprintf(buffer, buffer_size, "%lluB-blocks", block_size);
}

static int health_level_from_percent(double value)
{
    if (value >= CRITICAL_THRESHOLD) {
        return 2;
    }

    if (value >= WARNING_THRESHOLD) {
        return 1;
    }

    return 0;
}

static const char *health_status_from_level(int level)
{
    if (level >= 2) {
        return "CRITICAL";
    }

    if (level == 1) {
        return "WARNING";
    }

    return "OK";
}

static int max_int(int lhs, int rhs)
{
    return lhs > rhs ? lhs : rhs;
}

static int print_summary(const char *path, const display_options *display)
{
    diag_fs_info info;
    char total[32];
    char used[32];
    char available[32];
    char size_header[32];

    if (diag_fs_get_info(path, &info) != 0) {
        diag_print_error("bbfscheck", "failed to read filesystem summary", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    format_size_header(size_header, sizeof(size_header), display);
    format_fs_blocks(total, sizeof(total), &info, info.total_blocks, display);
    format_fs_blocks(used, sizeof(used), &info, info.used_blocks, display);
    format_fs_blocks(available, sizeof(available), &info, info.free_blocks, display);

    printf("%-20s %12s %12s %12s %5s %-8s %s\n",
           "Filesystem", size_header, "Used", "Available", "Use%", "Status",
           "Mounted on");
    printf("%-20s %12s %12s %12s %4.0f%% %-8s %s\n",
           info.filesystem,
           total,
           used,
           available,
           info.block_usage_percent,
           diag_health_status(WARNING_THRESHOLD, CRITICAL_THRESHOLD,
                              info.block_usage_percent),
           info.mount_point);

    return 0;
}

static int print_inode(const char *path)
{
    diag_fs_info info;

    if (diag_fs_get_info(path, &info) != 0) {
        diag_print_error("bbfscheck", "failed to read inode summary", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    printf("%-20s %12s %12s %12s %5s %-8s %s\n",
           "Filesystem", "Inodes", "IUsed", "IFree", "IUse%", "Status",
           "Mounted on");
    printf("%-20s %12lu %12lu %12lu %4.0f%% %-8s %s\n",
           info.filesystem,
           info.total_inodes,
           info.used_inodes,
           info.free_inodes,
           info.inode_usage_percent,
           diag_health_status(WARNING_THRESHOLD, CRITICAL_THRESHOLD,
                              info.inode_usage_percent),
           info.mount_point);

    return 0;
}

static int print_scan(const char *path, int max_depth, unsigned long small_file_size, const display_options *display)
{
    diag_fs_scan_info info;
    int rc;
    char total_size[32];

    rc = diag_fs_scan_path(path, max_depth, small_file_size, &info);
    if (rc != 0 && info.files == 0 && info.directories == 0 && info.symlinks == 0 &&
        info.other == 0 && info.skipped_special == 0) {
        diag_print_error("bbfscheck", "failed to scan path", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    format_scan_size(total_size, sizeof(total_size), info.total_size, display);

    printf("%-20s %12s %8s %8s %8s %8s %12s %8s %8s %-8s\n",
           "Path", "Size", "Files", "Dirs", "Links", "Other", "Small files",
           "Errors", "Skipped", "Status");
    printf("%-20s %12s %8lu %8lu %8lu %8lu %12lu %8lu %8lu %-8s\n",
           path,
           total_size,
           info.files,
           info.directories,
           info.symlinks,
           info.other,
           info.small_files,
           info.errors,
           info.skipped_special,
           info.errors == 0 ? "OK" : "WARNING");

    return info.errors == 0 ? 0 : BBFSCHECK_EXIT_READ_FAILURE;
}

static int print_check(const char *path, int max_depth, unsigned long small_file_size,
                       const display_options *display)
{
    diag_fs_info fs_info;
    diag_fs_scan_info scan_info;
    int scan_rc;
    int block_level;
    int inode_level;
    int small_file_level;
    int overall_level;
    double small_file_ratio;
    char total[32];
    char used[32];
    char available[32];
    char scan_size[32];
    char size_header[32];

    if (diag_fs_get_info(path, &fs_info) != 0) {
        diag_print_error("bbfscheck", "failed to read filesystem summary", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    scan_rc = diag_fs_scan_path(path, max_depth, small_file_size, &scan_info);
    if (scan_rc != 0 && scan_info.files == 0 && scan_info.directories == 0 &&
            scan_info.symlinks == 0 && scan_info.other == 0 &&
            scan_info.skipped_special == 0) {
        diag_print_error("bbfscheck", "failed to scan path", path);
        return BBFSCHECK_EXIT_READ_FAILURE;
    }

    small_file_ratio = 0.0;
    if(scan_info.files != 0) {
        small_file_ratio = ((double)scan_info.small_files / (double)scan_info.files) * 100.0;
    }

    block_level = health_level_from_percent(fs_info.block_usage_percent);
    inode_level = health_level_from_percent(fs_info.inode_usage_percent);
    small_file_level = 0;
    if (small_file_ratio >= SMALL_FILE_RATIO_CRITICAL) {
        small_file_level = 2;
    } else if (small_file_ratio >= SMALL_FILE_RATIO_WARNING) {
        small_file_level = 1;
    }

   overall_level = max_int(block_level, inode_level);
    overall_level = max_int(overall_level, small_file_level);
    if (scan_info.errors != 0 && overall_level < 1) {
        overall_level = 1;
    }

    format_size_header(size_header, sizeof(size_header), display);
    format_fs_blocks(total, sizeof(total), &fs_info, fs_info.total_blocks, display);
    format_fs_blocks(used, sizeof(used), &fs_info, fs_info.used_blocks, display);
    format_fs_blocks(available, sizeof(available), &fs_info, fs_info.free_blocks, display);
    format_scan_size(scan_size, sizeof(scan_size), scan_info.total_size, display);

    format_size_header(size_header, sizeof(size_header), display);
    format_fs_blocks(total, sizeof(total), &fs_info, fs_info.total_blocks, display);
    format_fs_blocks(used, sizeof(used), &fs_info, fs_info.used_blocks, display);
    format_fs_blocks(available, sizeof(available), &fs_info, fs_info.free_blocks, display);
    format_scan_size(scan_size, sizeof(scan_size), scan_info.total_size, display);

    printf("Path: %s\n", path);
    printf("Filesystem: %s\n", fs_info.filesystem);
    printf("Mounted on: %s\n", fs_info.mount_point);
    printf("Overall status: %s\n\n", health_status_from_level(overall_level));

    printf("Filesystem usage:\n");
    printf("%-10s %12s %12s %12s %7s %-8s\n",
           "Resource", size_header, "Used", "Available", "Use%", "Status");
    printf("%-10s %12s %12s %12s %6.0f%% %-8s\n",
           "Blocks", total, used, available, fs_info.block_usage_percent,
           health_status_from_level(block_level));
    printf("%-10s %12lu %12lu %12lu %6.0f%% %-8s\n\n",
           "Inodes", fs_info.total_inodes, fs_info.used_inodes, fs_info.free_inodes,
           fs_info.inode_usage_percent, health_status_from_level(inode_level));

    printf("Directory scan:\n");
    printf("%-14s %s\n", "Total size:", scan_size);
    printf("%-14s %lu\n", "Files:", scan_info.files);
    printf("%-14s %lu\n", "Directories:", scan_info.directories);
    printf("%-14s %lu\n", "Symlinks:", scan_info.symlinks);
    printf("%-14s %lu\n", "Other:", scan_info.other);
    printf("%-14s %lu (%.1f%% of files, threshold <= %lu bytes)\n",
           "Small files:", scan_info.small_files, small_file_ratio, small_file_size);
    printf("%-14s %lu\n", "Errors:", scan_info.errors);
    printf("%-14s %lu\n\n", "Skipped:", scan_info.skipped_special);

    printf("Reasons:\n");
    if (overall_level == 0) {
        printf("  - No filesystem usage threshold exceeded.\n");
    } else {
        if (block_level != 0) {
            printf("  - Block usage is %.0f%%, status=%s.\n",
                   fs_info.block_usage_percent, health_status_from_level(block_level));
        }
        if (inode_level != 0) {
            printf("  - Inode usage is %.0f%%, status=%s.\n",
                   fs_info.inode_usage_percent, health_status_from_level(inode_level));
        }
        if (small_file_level != 0) {
            printf("  - Small file ratio is %.1f%%, status=%s.\n",
                   small_file_ratio, health_status_from_level(small_file_level));
        }
        if (scan_info.errors != 0) {
            printf("  - Directory scan encountered %lu error(s).\n", scan_info.errors);
        }
    }

    if (scan_info.skipped_special != 0) {
        printf("  - Skipped %lu virtual/special path(s) during safe scan.\n",
               scan_info.skipped_special);
    }

    if (overall_level >= 2) {
        return BBFSCHECK_EXIT_CRITICAL;
    }

    if (overall_level == 1) {
        return BBFSCHECK_EXIT_GENERAL;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    int mode_count;
    const char *summary_path;
    const char *inode_path;
    const char *scan_path;
    const char *check_path;
    int max_depth;
    unsigned long small_file_size;
    display_options display;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    summary_path = NULL;
    inode_path = NULL;
    scan_path = NULL;
    check_path = NULL;
    max_depth = -1;
    small_file_size = DEFAULT_SMALL_FILE_SIZE;
    display.mode = DISPLAY_BLOCKS;
    display.block_size = DEFAULT_BLOCK_SIZE;

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
        } else if (strcmp(argv[i], "--check") == 0) {
            if (++i >= argc) {
                diag_print_error("bbfscheck", "missing path for --check", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            check_path = argv[i];
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
        } else if (strcmp(argv[i], "-h") == 0) {
            display.mode = DISPLAY_HUMAN_1024;
        } else if (strcmp(argv[i], "-H") == 0) {
            display.mode = DISPLAY_HUMAN_1000;
        } else if (strcmp(argv[i], "-B") == 0 || strcmp(argv[i], "--block-size") == 0) {
            unsigned long long parsed_size;

            if (++i >= argc || parse_size_bytes(argv[i], &parsed_size) != 0) {
                diag_print_error("bbfscheck", "invalid value for block size", NULL);
                return BBFSCHECK_EXIT_INVALID_ARGUMENT;
            }
            display.mode = DISPLAY_BLOCKS;
            display.block_size = parsed_size;
        } else {
            diag_print_error("bbfscheck", "unknown option", argv[i]);
            return BBFSCHECK_EXIT_INVALID_ARGUMENT;
        }
    }

    mode_count = (summary_path != NULL) + (inode_path != NULL) + 
                 (scan_path != NULL) + (check_path != NULL);
    if (mode_count != 1) {
        diag_print_error("bbfscheck", "choose exactly one mode",
                         "--summary, --inode, --scan, or --check");
        return BBFSCHECK_EXIT_INVALID_ARGUMENT;
    }

    if (summary_path != NULL) {
        return print_summary(summary_path, &display);
    }

    if (inode_path != NULL) {
        return print_inode(inode_path);
    }

    if (scan_path != NULL) {
        return print_scan(scan_path, max_depth, small_file_size, &display);
    }

    return print_check(check_path, max_depth, small_file_size, &display);
}
