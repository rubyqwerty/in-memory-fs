#include "fs_operations.hpp"

#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::level_enum::trace);

    return fuse_main(argc, argv, &file_system::GetCoreOperations(), NULL);
}