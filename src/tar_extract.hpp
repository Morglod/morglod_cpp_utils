#pragma once

#include <memory>

void tar_extract(const char *filename, char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path = 0);

void tar_extract_tgz_small_from_file(
    const char *filename,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path = 0
);

void tar_extract_nodeflate_from_memory(
    const char* input_memory, size_t input_size,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path = 0
);