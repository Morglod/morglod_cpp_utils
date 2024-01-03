#include "./fs_preallocate_file.hpp"

#include "./profiler.hpp"

#ifdef __APPLE__

#include <fcntl.h>

void fs_preallocate_file(int fd, off_t offset, off_t len) {
    _PROFILER_AUTO();

    fstore_t fstore;
    fstore.fst_flags = F_ALLOCATECONTIG;
    fstore.fst_posmode = F_PEOFPOSMODE;
    fstore.fst_offset = 0;
    fstore.fst_length = len + offset;

    // Based on https://api.kde.org/frameworks/kcoreaddons/html/posix__fallocate__mac_8h_source.html
    int rc = fcntl(fd, F_PREALLOCATE, &fstore);
    if (rc != 0) {
        fstore.fst_flags = F_ALLOCATEALL;
        fcntl(fd, F_PREALLOCATE, &fstore);
    }
}

#endif

#ifdef __linux__

#include <fcntl.h>

void fs_preallocate_file(int fd, off_t offset, off_t len) {
    _PROFILER_AUTO();

    fallocate(fd, 0, (int64_t)offset, len);
}

#endif