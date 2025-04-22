#include "fs_manager.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>

static int NodeId = 2;

struct NodeStructure
{
    std::vector<std::string> full_path{};
    std::vector<std::string> subdirectories{};
    std::string name;
};

static constexpr auto GetRootNodeId() { return 1; }

/// @brief По пути к файлу получить структуру пути
/// @param path Путь до файла
/// @return Массив из поддирикторий и имя узла
static NodeStructure GetNodeStructure(const std::string &path)
{
    auto nodes_view = path | std::ranges::views::split('/') |
                      std::ranges::views::transform(
                          [](auto &&rng) { return std::string_view(&*rng.begin(), std::ranges::distance(rng)); });

    std::vector<std::string> subdirectories(nodes_view.begin(), nodes_view.end());

    const auto file_name = *(subdirectories.end() - 1);

    subdirectories.erase(subdirectories.begin());

    const auto full_path = subdirectories;

    subdirectories.erase(subdirectories.end() - 1);

    return {full_path, subdirectories, file_name};
}

template <auto type> void FsManager::MakeNode(const std::string &path, const mode_t &mode)
{
    const auto node = GetNodeStructure(path);

    const auto node_id = GetNodeId(node.subdirectories);

    if (!node_id)
        throw std::runtime_error(std::format("No such file or directory: {}", path));

    const auto create_time = time(0);

    db.nodes.push_back(
        {.inode_id = NodeId, .type = type, .atime = create_time, .mtime = create_time, .ctime = create_time});
    db.entry.push_back({node_id.value(), node.name, NodeId++});
}

std::optional<int> FsManager::GetNodeId(const auto start, const auto end, int parent_node)
{
    if (start == end)
        return parent_node;

    for (const auto &node : db.entry)
    {
        if (node.parent_node_id == parent_node && *start == node.name)
        {
            return GetNodeId(start + 1, end, node.child_node_id);
        }
    }

    return {};
}

std::optional<int> FsManager::GetNodeId(const std::vector<std::string> &subdirectories)
{
    int parent_node = GetRootNodeId();

    return GetNodeId(subdirectories.begin(), subdirectories.end(), parent_node);
}

std::vector<Inode>::iterator FsManager::GetAttribute(const std::string &path)
{
    const auto node = GetNodeStructure(path);

    const auto node_id = GetNodeId(node.full_path);

    if (!node_id)
        throw std::runtime_error(std::format("No such file or directory: {}", path));

    return std::ranges::find(db.nodes, node_id.value(), &Inode::inode_id);
}

void FsManager::MakeDirectory(const std::string &path) { MakeNode<S_IFDIR>(path, {}); }

void FsManager::RemoveDirectory(const std::string &path) {}

void FsManager::MakeFile(const std::string &path, const mode_t mode) { MakeNode<S_IFREG>(path, mode); }

void FsManager::MakeTimestamps(const std::string &path, const struct timespec ts[2])
{
    auto node = GetAttribute(path);

    node->atime = ts[0].tv_nsec == UTIME_NOW ? time(0) : node->atime;
    node->mtime = ts[1].tv_nsec == UTIME_NOW ? time(0) : node->mtime;
}

Bytes FsManager::ReadFile(const int node_id, const int size, const int offset)
{
    auto node = std::ranges::find(db.nodes, node_id, &Inode::inode_id);

    return node->data;
}

void FsManager::WriteFile(const int node_id, const std::string &buffer, const int offset)
{
    auto node = std::ranges::find(db.nodes, node_id, &Inode::inode_id);

    node->data = std::string(buffer.data());
    node->size = buffer.size();
}

std::vector<std::string> FsManager::ReadDirectory(const std::string &path)
{
    if (path == "")
        return {};
    const auto node = GetNodeStructure(path);

    const auto node_id = path == "/" ? GetRootNodeId() : GetNodeId(node.full_path);

    if (!node_id)
        throw std::runtime_error(std::format("No such file or directory: {}", path));

    auto nodes = db.entry | std::views::filter([&](const auto &dir) { return dir.parent_node_id == node_id.value(); }) |
                 std::views::transform([&](const auto &dir) { return dir.name; });

    return {nodes.begin(), nodes.end()};
}
