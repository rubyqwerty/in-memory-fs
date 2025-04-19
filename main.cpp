#include <fcntl.h>
#include <sys/stat.h>

#include <iterator>
#define FUSE_USE_VERSION 32

#include <fuse3/fuse.h>

#include "fs_manager.hpp"
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <vector>

FsManager manager;

static int memfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
try
{
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const auto node = manager.GetAttribute(path);

    stbuf->st_mode = node.type;
    stbuf->st_size = node.size;
    stbuf->st_atime = node.atime;
    stbuf->st_mtime = node.mtime;
    stbuf->st_ctime = node.ctime;

    return 0;
}
catch (...)
{
    return -ENOENT;
}

static int memfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, struct fuse_file_info *,
                         enum fuse_readdir_flags)
{
    filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    
    for (const auto &node : manager.ReadDirectory(path))
    {
        filler(buf, node.data(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}

static int memfs_mkdir(const char *path, mode_t)
try
{
    manager.MakeDirectory(path);
    return 0;
}
catch (...)
{
    return -ENOENT;
}

static int memfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) { return -ENOENT; }

static int memfs_open(const char *path, struct fuse_file_info *fi) { return -ENOENT; }

static int memfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return -ENOENT;
}

static int memfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return -ENOENT;
}

static const struct fuse_operations memfs_oper = {
    .getattr = memfs_getattr, .mkdir = memfs_mkdir, .readdir = memfs_readdir,

    // .create = memfs_create,

};

int main(int argc, char *argv[])
{
    manager.MakeDirectory("/test");
    manager.MakeDirectory("/test2");
    manager.MakeDirectory("/test2/dir");
    manager.MakeDirectory("/test2/dir2");

    const auto d = manager.ReadDirectory("/");
    const auto d2 = manager.ReadDirectory("/test2");
    const auto d3 = manager.ReadDirectory("/test2/dir");
    return fuse_main(argc, argv, &memfs_oper, NULL);
}