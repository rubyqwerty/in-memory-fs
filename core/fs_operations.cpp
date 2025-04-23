#include "fs_operations.hpp"

#include "spdlog/spdlog.h"

namespace file_system
{
    static FsManager manager;

    struct Meta
    {
        Inode node;
        int offset;
    };

    /// @brief Получить дескриптор файла
    /// @param fi Информация о файле
    /// @return Метаинформация об узле
    static auto GetMetaInfo(const fuse_file_info *fi)
    {
        const auto fh = reinterpret_cast<const Meta *>(fi->fh);

        if (!fh)
            throw std::runtime_error("Ошибка получения дескриптора файла");

        spdlog::info("Получен дескриптор файла. Идентификатор файла {}, смещение: {} ", fh->node.inode_id, fh->offset);

        return fh;
    }

    static auto MakeFileDescriptor(const auto &inode) { return reinterpret_cast<uint64_t>(new Meta{*inode}); }

    static auto ReleaseFileDescriptor(const fuse_file_info *fi)
    {
        const auto fh = GetMetaInfo(fi);

        delete fh;
    }

    int getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
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

    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, struct fuse_file_info *,
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

    int mkdir(const char *path, mode_t)
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

    int create(const char *path, mode_t mode, struct fuse_file_info *fi)
    try
    {
        spdlog::info("Создание файла. Путь: {}", path);

        manager.MakeFile(path, mode);

        const auto node = manager.GetAttribute(path);

        fi->fh = MakeFileDescriptor(node);

        return 0;
    }
    catch (std::exception &ex)
    {
        spdlog::error("Ошибка создания файла: {}, {}", path, ex.what());
        return -ENOENT;
    }

    int open(const char *path, struct fuse_file_info *fi)
    try
    {
        spdlog::info("Открытие файла файла: {}", path);

        const auto node = manager.GetAttribute(path);

        fi->fh = MakeFileDescriptor(node);

        return 0;
    }
    catch (std::exception &ex)
    {
        spdlog::error("Ошибка открытия файла: {}, {}", path, ex.what());
        return -ENOENT;
    }

    int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
    try
    {
        spdlog::info("Запись в файл: {}. Размер: {}", path, size);

        const auto fh = GetMetaInfo(fi);

        manager.WriteFile(fh->node.inode_id, std::string(buf, size - 1), offset);

        return size;
    }
    catch (std::exception &ex)
    {
        spdlog::error("Ошибка записи в файл: {}, {}", path, ex.what());
        return -ENOENT;
    }

    int release(const char *path, struct fuse_file_info *fi)
    {
        spdlog::info("Освобождение файла: {}", path);

        ReleaseFileDescriptor(fi);

        return 0;
    }

    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
    try
    {
        spdlog::info("Чтение из файла: {}", path);

        const auto fh = GetMetaInfo(fi);

        auto buffer = manager.ReadFile(fh->node.inode_id, size, offset);

        std::copy(buffer.data(), buffer.data() + buffer.size(), buf);

        return size;
    }
    catch (std::exception &ex)
    {
        spdlog::error("Ошибка чтения из файла: {}, {}", path, ex.what());
        return -ENOENT;
    }

    int utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi)
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

    int rename(const char *old_path, const char *new_path, unsigned int flags) { return 0; }

    int truncate(const char *path, off_t size, fuse_file_info *fi) { return 0; }

    int unlink(const char *path) { return 0; }

    int rmdir(const char *path) { return 0; }

    fuse_operations &GetCoreOperations()
    {
        static struct fuse_operations operations = {
            .getattr = getattr,
            .mkdir = mkdir,
            .unlink = unlink,
            .rmdir = rmdir,
            .rename = rename,
            .truncate = truncate,
            .open = open,
            .read = read,
            .write = write,
            .release = release,
            .readdir = readdir,
            .create = create,
            .utimens = utimens,
        };
        return operations;
    }
}