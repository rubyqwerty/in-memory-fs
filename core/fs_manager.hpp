#include "spdlog/spdlog.h"
#include <cstdint>
#include <fcntl.h>
#include <optional>
#include <span>
#include <string>
#include <sys/stat.h>
#include <vector>

/// @brief Алиас на набор байты
using Bytes = std::string;

struct Inode
{
    int inode_id;
    mode_t type;
    int size;
    time_t atime;
    time_t mtime;
    time_t ctime;
    std::string data{};
};

struct Directory
{
    int parent_node_id{};
    std::string name{};
    int child_node_id{};
};

struct Db
{
    std::vector<Directory> entry{};
    std::vector<Inode> nodes{{1, S_IFREG}};
};

/// @brief Менеджер управления файловой системой
class FsManager
{
public:
    /// @brief Получить атрибуты файла
    /// @param path Путь до файла
    /// @return Структура с описанием атрибутов файла
    std::vector<Inode>::iterator GetAttribute(const std::string &path);

    /// @brief Создать новую директорию
    /// @param path Путь до директории
    void MakeDirectory(const std::string &path);

    /// @brief Удалить директорию
    /// @param path Путь до директории
    void RemoveDirectory(const std::string &path);

    /// @brief Создать файл
    /// @param path Путь до файла
    /// @param mode Атрибуты файла
    void MakeFile(const std::string &path, const mode_t mode);

    /// @brief Установить временные метки
    /// @param path Путь до узла
    /// @param ts Временные метки
    void MakeTimestamps(const std::string &path, const struct timespec ts[2]);

    /// @brief Прочитать файл
    /// @param node_id Идентификатор файла
    /// @param size Размер считываемых данных
    /// @param offset Смещение относительно начала файла
    /// @return Считанные данные
    Bytes ReadFile(const int node_id, const int size, const int offset);

    /// @brief Записать данные в файл
    /// @param node_id Идентификатор файла
    /// @param buffer Буфер с записываемыми данными
    /// @param offset Смещение относительно начала
    void WriteFile(const int node_id, const std::string &buffer, const int offset);

    /// @brief Прочитать директорию
    /// @param path Путь до директории
    /// @return Информация о директории
    std::vector<std::string> ReadDirectory(const std::string &path);

private:
    template <auto type> void MakeNode(const std::string &path, const mode_t &mode);

    std::optional<int> GetNodeId(const auto start, const auto end, int parent_node);

    std::optional<int> GetNodeId(const std::vector<std::string> &path);

private:
    Db db{};
};