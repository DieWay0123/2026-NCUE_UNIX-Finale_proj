#include "diag_common.h"

#ifdef _WIN32

#include <errno.h>
#include <string.h>

int diag_fs_get_info(const char *path, diag_fs_info *info)
{
    (void)path;

    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }

    errno = ENOSYS;
    return -1;
}

int diag_fs_scan_path(const char *path, int max_depth, unsigned long small_file_size,
                      diag_fs_scan_info *info)
{
    (void)path;
    (void)max_depth;
    (void)small_file_size;

    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }

    errno = ENOSYS;
    return -1;
}

#else

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int is_special_virtual_path(const char *path)
{
    static const char *special_paths[] = {
        "/proc",
        "/sys",
        "/dev",
        "/run",
    };
    size_t i;

    if (path == NULL) {
        return 0;
    }

    for (i = 0; i < sizeof(special_paths) / sizeof(special_paths[0]); i++) {
        size_t len = strlen(special_paths[i]);

        if (strncmp(path, special_paths[i], len) == 0 &&
            (path[len] == '\0' || path[len] == '/')) {
            return 1;
        }
    }

    return 0;
}

int diag_fs_get_info(const char *path, diag_fs_info *info)
{
    struct statvfs st;
    unsigned long used_blocks;
    unsigned long used_inodes;

    if (path == NULL || info == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (statvfs(path, &st) != 0) {
        return -1;
    }

    memset(info, 0, sizeof(*info));
    used_blocks = st.f_blocks - st.f_bfree;
    used_inodes = st.f_files >= st.f_ffree ? st.f_files - st.f_ffree : 0;

    info->total_blocks = st.f_blocks;
    info->used_blocks = used_blocks;
    info->free_blocks = st.f_bavail;
    info->total_inodes = st.f_files;
    info->used_inodes = used_inodes;
    info->free_inodes = st.f_ffree;
    info->block_size = st.f_frsize != 0 ? st.f_frsize : st.f_bsize;

    if (st.f_blocks != 0) {
        info->block_usage_percent = ((double)used_blocks / (double)st.f_blocks) * 100.0;
    }

    if (st.f_files != 0) {
        info->inode_usage_percent = ((double)used_inodes / (double)st.f_files) * 100.0;
    }

    return 0;
}

static int join_path(char *buffer, size_t buffer_size, const char *parent, const char *name)
{
    size_t parent_len;

    parent_len = strlen(parent);
    if (parent_len > 0 && parent[parent_len - 1] == '/') {
        return snprintf(buffer, buffer_size, "%s%s", parent, name) >= (int)buffer_size ? -1 : 0;
    }

    return snprintf(buffer, buffer_size, "%s/%s", parent, name) >= (int)buffer_size ? -1 : 0;
}

static int scan_recursive(const char *path, int depth, int max_depth,
                          unsigned long small_file_size, diag_fs_scan_info *info)
{
    struct stat st;
    DIR *dir;
    struct dirent *entry;

    if (is_special_virtual_path(path)) {
        info->skipped_special++;
        return 0;
    }

    if (lstat(path, &st) != 0) {
        info->errors++;
        return -1;
    }

    if (S_ISREG(st.st_mode)) {
        info->files++;
        info->total_size += (unsigned long long)st.st_size;
        if ((unsigned long long)st.st_size <= (unsigned long long)small_file_size) {
            info->small_files++;
        }
        return 0;
    }

    if (S_ISLNK(st.st_mode)) {
        info->symlinks++;
        return 0;
    }

    if (!S_ISDIR(st.st_mode)) {
        info->other++;
        return 0;
    }

    info->directories++;
    if (max_depth >= 0 && depth >= max_depth) {
        return 0;
    }

    dir = opendir(path);
    if (dir == NULL) {
        info->errors++;
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char child_path[PATH_MAX];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (join_path(child_path, sizeof(child_path), path, entry->d_name) != 0) {
            info->errors++;
            continue;
        }

        scan_recursive(child_path, depth + 1, max_depth, small_file_size, info);
    }

    if (closedir(dir) != 0) {
        info->errors++;
        return -1;
    }

    return info->errors == 0 ? 0 : -1;
}

int diag_fs_scan_path(const char *path, int max_depth, unsigned long small_file_size,
                      diag_fs_scan_info *info)
{
    if (path == NULL || info == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(info, 0, sizeof(*info));
    return scan_recursive(path, 0, max_depth, small_file_size, info);
}

#endif
