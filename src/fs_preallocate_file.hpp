#pragma once

#include <memory>

void fs_preallocate_file(int fd, off_t offset, off_t len);
