#include <fcntl.h>

#include <iterator>
#define FUSE_USE_VERSION 32

#include <fuse.h>

#include "fs_manager.hpp"
#include "spdlog/spdlog.h"
#include <cstring>
#include <ctime>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <string>
#include <vector>

FsManager manager;

struct Meta
{
    Inode node;
    int offset;
};

static int memfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
try
{
    spdlog::debug("Запрос атрибутов. Путь: {}", path);

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const auto node = manager.GetAttribute(path);

    stbuf->st_mode = node->type;
    stbuf->st_size = node->size;
    stbuf->st_atime = node->atime;
    stbuf->st_mtime = node->mtime;
    stbuf->st_ctime = node->ctime;

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка получения атрибутов для: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, struct fuse_file_info *,
                         enum fuse_readdir_flags)
try
{
    spdlog::info("Чтение директории. Путь: {}", path);

    filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    for (const auto &node : manager.ReadDirectory(path))
    {
        filler(buf, node.data(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка чтения директории: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_mkdir(const char *path, mode_t)
try
{
    spdlog::info("Создание директории. Путь: {}", path);

    manager.MakeDirectory(path);

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка создания директории: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
try
{
    spdlog::info("Создание файла. Путь: {}", path);

    manager.MakeFile(path, mode);

    const auto node = manager.GetAttribute(path);

    fi->fh = reinterpret_cast<uint64_t>(new Meta{*node});

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка создания файла: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_open(const char *path, struct fuse_file_info *fi)
try
{
    spdlog::info("Открытие файла файла: {}", path);

    const auto node = manager.GetAttribute(path);

    fi->fh = reinterpret_cast<uint64_t>(new Meta{*node});

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка открытия файла: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
try
{
    spdlog::info("Запись в файл: {}. Размер: {}", path, size);

    const auto fh = reinterpret_cast<const Meta *>(fi->fh);

    if (!fh)
        return -ENOENT;

    manager.WriteFile(fh->node.inode_id, std::string(buf, size - 1), offset);

    return size;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка записи в файл: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_release(const char *path, struct fuse_file_info *fi)
{
    spdlog::info("Освобождение файла: {}", path);

    const auto fh = reinterpret_cast<const Meta *>(fi->fh);

    delete fh;

    return 0;
}

static int memfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
try
{
    spdlog::info("Чтение из файла: {}", path);

    const auto fh = reinterpret_cast<const Meta *>(fi->fh);

    auto buffer = manager.ReadFile(fh->node.inode_id, size, offset);

    std::copy(buffer.data(), buffer.data() + buffer.size(), buf);

    return size;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка чтения из файла: {}, {}", path, ex.what());
    return -ENOENT;
}

static int memfs_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi)
try
{
    spdlog::info("Изменение временных меток: {}", path);
    manager.MakeTimestamps(path, ts);

    return 0;
}
catch (std::exception &ex)
{
    spdlog::error("Ошибка изменения временных меток: {}, {}", path, ex.what());
    return -ENOENT;
}

static const struct fuse_operations memfs_oper = {
    .getattr = memfs_getattr,
    .mkdir = memfs_mkdir,
    .open = memfs_open,
    .read = memfs_read,

    .write = memfs_write,
    .release = memfs_release,
    .readdir = memfs_readdir,

    .create = memfs_create,
    .utimens = memfs_utimens,

};

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::level_enum::trace);

    manager.MakeDirectory("/test");
    manager.MakeDirectory("/test2");
    manager.MakeDirectory("/test2/dir");
    manager.MakeDirectory("/test2/dir2");

    const auto d = manager.ReadDirectory("/");
    const auto d2 = manager.ReadDirectory("/test2");
    const auto d3 = manager.ReadDirectory("/test2/dir");
    return fuse_main(argc, argv, &memfs_oper, NULL);
}