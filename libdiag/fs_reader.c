#include "diag_common.h"

#ifdef _WIN32

#include <errno.h>
#include <string.h>

int diag_fs_get_info(const char *path, diag_fs_info *info){
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

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

static void copy_string(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, dest_size, "%s", src);
}

static void decode_mountinfo_field(char *text)
{
    char *read_pos;
    char *write_pos;

    read_pos = text;
    write_pos = text;
    while (*read_pos != '\0') {
        if (read_pos[0] == '\\' && read_pos[1] >= '0' && read_pos[1] <= '7' &&
            read_pos[2] >= '0' && read_pos[2] <= '7' &&
            read_pos[3] >= '0' && read_pos[3] <= '7') {
            *write_pos = (char)(((read_pos[1] - '0') << 6) |
                                ((read_pos[2] - '0') << 3) |
                                (read_pos[3] - '0'));
            read_pos += 4;
            write_pos++;
            continue;
        }

        *write_pos++ = *read_pos++;
    }
    *write_pos = '\0';
}

static int path_is_under_mount(const char *path, const char *mount_point)
{
    size_t mount_len;

    if (strcmp(mount_point, "/") == 0) {
        return 1;
    }

    mount_len = strlen(mount_point);
    return strncmp(path, mount_point, mount_len) == 0 &&
           (path[mount_len] == '\0' || path[mount_len] == '/');
}

static void fill_mount_info(const char *path, diag_fs_info *info)
{
    FILE *fp;
    char resolved_path[PATH_MAX];
    char line[1024];
    size_t best_len;

    if (realpath(path, resolved_path) == NULL) {
        copy_string(resolved_path, sizeof(resolved_path), path);
    }

    copy_string(info->filesystem, sizeof(info->filesystem), path);
    copy_string(info->mount_point, sizeof(info->mount_point), path);

    fp = fopen("/proc/self/mountinfo", "r");
    if (fp == NULL) {
        return;
    }

    best_len = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *mount_point;
        char *saveptr;
        char *token;
        char *mount_source;
        size_t count;
        size_t mount_len;

        count = 0;
        mount_point = NULL;
        mount_source = NULL;
        token = strtok_r(line, " \n", &saveptr);

        while (token != NULL) {
            if (count == 4) {
                mount_point = token;
            }

            if (strcmp(token, "-") == 0) {
                (void)strtok_r(NULL, " \n", &saveptr);
                mount_source = strtok_r(NULL, " \n", &saveptr);
                break;
            }

            count++;
            token = strtok_r(NULL, " \n", &saveptr);
        }

        if (mount_point == NULL || mount_source == NULL) {
            continue;
        }

        decode_mountinfo_field(mount_point);
        decode_mountinfo_field(mount_source);

        if (!path_is_under_mount(resolved_path, mount_point)) {
            continue;
        }

        mount_len = strlen(mount_point);
        if (mount_len < best_len) {
            continue;
        }

        best_len = mount_len;
        copy_string(info->filesystem, sizeof(info->filesystem), mount_source);
        copy_string(info->mount_point, sizeof(info->mount_point), mount_point);
    }

    fclose(fp);
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
    fill_mount_info(path, info);
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
        info->block_usage_percent = ((double)used_blocks / ((double)used_blocks+(double)st.f_bavail)) * 100.0;
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
