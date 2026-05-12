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

int diag_fs_get_mount_info(const char *path, diag_mount_info *info)
{
    (void)path;
    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }

    errno = ENOSYS;
    return -1;
}

int diag_fs_read_mounts(diag_mount_info *mounts, size_t capacity, size_t *count)
{
    (void)mounts;
    (void)capacity;

    if (count != NULL) {
        *count = 0;
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

static int option_list_has(const char *options, const char *wanted)
{
    size_t wanted_len;
    const char *pos;

    if (options == NULL || wanted == NULL) {
        return 0;
    }

    wanted_len = strlen(wanted);
    pos = options;

    while (*pos != '\0') {
        if (strncmp(pos, wanted, wanted_len) == 0 &&
            (pos[wanted_len] == '\0' || pos[wanted_len] == ',')) {
            return 1;
        }

        pos = strchr(pos, ',');
        if (pos == NULL) {
            break;
        }
        pos++;
    }

    return 0;
}

static int parse_mountinfo_line(char *line, diag_mount_info *info)
{
    char *saveptr;
    char *token;
    char *mount_id;
    char *parent_id;
    char *root;
    char *mount_point;
    char *options;
    char *fs_type;
    char *source;
    char *super_options;
    char fallback_source[] = "none";
    char fallback_super_options[] = "";
    size_t field;

    memset(info, 0, sizeof(*info));

    mount_id = NULL;
    parent_id = NULL;
    root = NULL;
    mount_point = NULL;
    options = NULL;
    fs_type = NULL;
    source = NULL;
    super_options = NULL;

    field = 0;
    token = strtok_r(line, " \n", &saveptr);
    while(token != NULL) {
        if (strcmp(token, "-") == 0) {
            fs_type = strtok_r(NULL, " \n", &saveptr);
            source = strtok_r(NULL, " \n", &saveptr);
            super_options = strtok_r(NULL, " \n", &saveptr);
            break;
        }

        if (field == 0) {
            mount_id = token;
        } else if (field == 1) {
            parent_id = token;
        } else if (field == 3) {
            root = token;
        } else if (field == 4) {
            mount_point = token;
        } else if (field == 5) {
            options = token;
        }

        field++;
        token = strtok_r(NULL, " \n", &saveptr);
    }

    if (mount_id == NULL || parent_id == NULL || root == NULL ||
        mount_point == NULL || options == NULL || fs_type == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (source == NULL) {
        source = fallback_source;
    }
    if (super_options == NULL) {
        super_options = fallback_super_options;
    }

    decode_mountinfo_field(root);
    decode_mountinfo_field(mount_point);
    decode_mountinfo_field(source);

    info->mount_id = atoi(mount_id);
    info->parent_id = atoi(parent_id);
    copy_string(info->root, sizeof(info->root), root);
    copy_string(info->mount_point, sizeof(info->mount_point), mount_point);
    copy_string(info->options, sizeof(info->options), options);
    copy_string(info->fs_type, sizeof(info->fs_type), fs_type);
    copy_string(info->source, sizeof(info->source), source);
    copy_string(info->super_options, sizeof(info->super_options), super_options);
    info->read_only = option_list_has(options, "ro");

    return 0;
}

static int resolve_path_for_mount(const char *path, char *buffer, size_t buffer_size)
{
    char cwd[PATH_MAX];

    if (realpath(path, buffer) != NULL) {
        return 0;
    }

    if (path[0] == '/') {
        copy_string(buffer, buffer_size, path);
        return 0;
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return -1;
    }

    if (snprintf(buffer, buffer_size, "%s/%s", cwd, path) >= (int)buffer_size) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}

int diag_fs_get_mount_info(const char *path, diag_mount_info *info)
{
    FILE *fp;
    char resolved_path[PATH_MAX];
    char line[8192];
    size_t best_len;
    int found;

    if (path == NULL || info == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (resolve_path_for_mount(path, resolved_path, sizeof(resolved_path)) != 0) {
        return -1;
    }

    fp = fopen("/proc/self/mountinfo", "r");
    if (fp == NULL) {
        return -1;
    }

    memset(info, 0, sizeof(*info));
    best_len = 0;
    found = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        diag_mount_info current;
        size_t mount_len;

        if (parse_mountinfo_line(line, &current) != 0) {
            continue;
        }

        if (!path_is_under_mount(resolved_path, current.mount_point)) {
            continue;
        }

        mount_len = strlen(current.mount_point);
        if (mount_len < best_len) {
            continue;
        }

        best_len = mount_len;
        *info = current;
        found = 1;
    }

    fclose(fp);

    if (!found) {
        errno = ENOENT;
        return -1;
    }

    return 0;
}

int diag_fs_read_mounts(diag_mount_info *mounts, size_t capacity, size_t *count)
{
    FILE *fp;
    char line[8192];
    size_t used;

    if (mounts == NULL || count == NULL || capacity == 0) {
        errno = EINVAL;
        return -1;
    }

    fp = fopen("/proc/self/mountinfo", "r");
    if (fp == NULL) {
        return -1;
    }

    used = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        diag_mount_info current;

        if (parse_mountinfo_line(line, &current) != 0) {
            continue;
        }

        if (used >= capacity) {
            fclose(fp);
            *count = used;
            errno = ENOSPC;
            return -1;
        }

        mounts[used++] = current;
    }

    fclose(fp);
    *count = used;
    return 0;
}

static void fill_mount_info(const char *path, diag_fs_info *info)
{
    diag_mount_info mount_info;
    if(diag_fs_get_mount_info(path, &mount_info) != 0) {
        return;
    }

    copy_string(info->filesystem, sizeof(info->filesystem), mount_info.source);
    copy_string(info->mount_point, sizeof(info->mount_point), mount_info.mount_point);
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
