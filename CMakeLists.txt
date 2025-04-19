cmake_minimum_required(VERSION 3.10.0)
project(in-memory-fs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINKER_TYPE MOLD)

add_executable(fs main.cpp fs_manager.cpp)

target_link_libraries(fs libfuse3.so)

