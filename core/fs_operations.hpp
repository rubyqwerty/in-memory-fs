#define FUSE_USE_VERSION 32

#include <fuse.h>

#include "fs_manager.hpp"

namespace file_system
{
    fuse_operations &GetCoreOperations();
}