#ifndef DIAG_COMMON_H
#define DIAG_COMMON_H

#include <stddef.h>

#define DIAG_COMM_LEN 256
#define DIAG_ADDR_LEN 64
#define DIAG_STATE_LEN 32
#define DIAG_PATH_LEN 256

typedef struct {
    int pid;
    int ppid;
    char comm[DIAG_COMM_LEN];
    unsigned long rss_kb;
    double cpu_percent;
} diag_proc_info;

typedef struct {
    char filesystem[DIAG_PATH_LEN];
    char mount_point[DIAG_PATH_LEN];
    unsigned long total_blocks;
    unsigned long used_blocks;
    unsigned long free_blocks;
    unsigned long total_inodes;
    unsigned long used_inodes;
    unsigned long free_inodes;
    unsigned long block_size;
    double block_usage_percent;
    double inode_usage_percent;
} diag_fs_info;

typedef struct {
    unsigned long files;
    unsigned long directories;
    unsigned long symlinks;
    unsigned long other;
    unsigned long small_files;
    unsigned long skipped_special;
    unsigned long errors;
    unsigned long long total_size;
} diag_fs_scan_info;

typedef struct {
    char local_addr[DIAG_ADDR_LEN];
    int local_port;
    char remote_addr[DIAG_ADDR_LEN];
    int remote_port;
    char state[DIAG_STATE_LEN];
} diag_tcp_conn;

int diag_fs_get_info(const char *path, diag_fs_info *info);
int diag_fs_scan_path(const char *path, int max_depth, unsigned long small_file_size,
                      diag_fs_scan_info *info);
void diag_print_error(const char *tool, const char *message, const char *detail);
const char *diag_health_status(double warning_threshold, double critical_threshold, double value);

#endif
