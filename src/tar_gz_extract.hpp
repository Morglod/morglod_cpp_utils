#pragma once

#include <memory>

int do_decompress(
	struct libdeflate_decompressor *decompressor,
	const uint8_t* compressed_data, size_t compressed_size,
	char** out_uncompressed_data, size_t* out_uncompressed_size
);

void tar_gz_extract(
    const char *filename,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path
);