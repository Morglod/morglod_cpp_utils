#include "./fs_create_dir.hpp"

#include "./log.hpp"
#include "./profiler.hpp"

#include <filesystem>

bool fs_create_dir(std::string const& file_path) {
    _PROFILER_AUTO();
    std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
}

bool fs_create_dir(const char* file_path) {
    _PROFILER_AUTO();
    return std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
}
