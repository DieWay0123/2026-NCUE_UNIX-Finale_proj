#ifndef DIAG_COMMON_H
#define DIAG_COMMON_H

#include <stddef.h>

#define DIAG_COMM_LEN 256
#define DIAG_ADDR_LEN 64
#define DIAG_STATE_LEN 32

typedef struct {
    int pid;
    int ppid;
    char comm[DIAG_COMM_LEN];
    unsigned long rss_kb;
    double cpu_percent;
} diag_proc_info;

typedef struct {
    unsigned long total_blocks;
    unsigned long used_blocks;
    unsigned long free_blocks;
    unsigned long total_inodes;
    unsigned long used_inodes;
    double inode_usage_percent;
} diag_fs_info;

typedef struct {
    char local_addr[DIAG_ADDR_LEN];
    int local_port;
    char remote_addr[DIAG_ADDR_LEN];
    int remote_port;
    char state[DIAG_STATE_LEN];
} diag_tcp_conn;

void diag_print_error(const char *tool, const char *message, const char *detail);
const char *diag_health_status(double warning_threshold, double critical_threshold, double value);

#endif
