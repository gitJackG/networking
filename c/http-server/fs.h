#pragma once

#include <linux/limits.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "string_ops.h"

typedef struct {
    bool exists;
} fs_metadata;


fs_metadata get_fs_metadata(string filename) {
    char buf[PATH_MAX];
    struct stat st;
    fs_metadata metadata;
    if (filename.len + 1 > PATH_MAX) {
        metadata.exists = false;
        return metadata;
    }
    memset(buf, 0, sizeof(buf));
    memcpy(buf, filename.data, filename.len);
    if (stat(buf, &st) < 0) {
        metadata.exists = false;
        return metadata;
    }
    metadata.exists = true;
    return metadata;
}
