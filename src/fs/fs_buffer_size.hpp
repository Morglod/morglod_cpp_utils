#pragma once

#include <stdio.h>
#include <stdlib.h>

#if 1 == 0
// linux

#include <unistd.h>
#include <fcntl.h>

inline int fs_get_io_effecient_buffer_size() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int bufsize = fcntl(fd[0], F_GETPIPE_SZ);
    if (bufsize == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    printf("Kernel buffer size for anonymous pipe: %d bytes\n", bufsize);

    close(fd[0]);
    close(fd[1]);

    return bufsize;
}
#endif

#if 1 == 1
// darwin

#include <sys/param.h>
#include <sys/mount.h>

inline int fs_get_io_effecient_buffer_size() {
    struct statfs stat_buf;
    const char *path = "./";

    if (statfs(path, &stat_buf) == -1) {
        perror("statfs");
        return EXIT_FAILURE;
    }

    unsigned long block_size = stat_buf.f_iosize;
    return block_size;
}

#endif