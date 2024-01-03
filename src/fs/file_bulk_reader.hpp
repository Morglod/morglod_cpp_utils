#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "../log.hpp"
#include "../profiler.hpp"
#include "../no_inline.hpp"

#define _FILE_STREAM_DARWIN

#ifdef _FILE_STREAM_DARWIN

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/event.h>
#include <unistd.h>

#endif

constexpr size_t _file_bulk_reader_CHUNK_SIZE = 4 * 1024;

namespace {
struct _file_bulk_reader_empty_ext {};
}

template<typename FileBufExt = _file_bulk_reader_empty_ext>
struct _file_bulk_reader_base {
    int num_files = 0;
    int max_files;

    struct _file_buf_base {
        char* buf;
        /** same buf but with read offset */
        char* buf_read_it;
        size_t size = 0;
    };

    struct file_buf : _file_buf_base, FileBufExt {};

    /** sizeof max_files */
    file_buf* file_bufs = nullptr;

    inline void set_file_data(size_t i, FileBufExt&& data) {
        new (((char*)&file_bufs[i]) + sizeof(_file_buf_base)) FileBufExt(data);
    }
};

#ifdef _FILE_STREAM_DARWIN

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/event.h>
#include <unistd.h>

const struct timespec file_bulk_reader_push_file_timeout = {0, 0};

struct _file_bulk_reader_darwin {
    int kq = 0;

    /** sizeof max_files */
    struct kevent* events = nullptr;
};

template<typename FileBufExt = _file_bulk_reader_empty_ext>
struct file_bulk_reader : _file_bulk_reader_darwin, _file_bulk_reader_base<FileBufExt> {
    void init(int max_files);
    void push_file(const char* file_path);

    void wait_open_read_all();
    void wait_open_all();
    void close_fds();
    void free();
};

template<typename FileBufExt>
inline void file_bulk_reader<FileBufExt>::init(int max_files) {
    _PROFILER_AUTO();
    
    const size_t alloc_size = (sizeof(struct kevent) + sizeof(typename _file_bulk_reader_base<FileBufExt>::file_buf)) * max_files;

    this->kq = kqueue();

    this->max_files = max_files;

    char* mem = (char*)malloc(alloc_size);
    memset(mem, 0, alloc_size);
    this->events = (struct kevent*)mem;
    this->file_bufs = (typename _file_bulk_reader_base<FileBufExt>::file_buf *)(mem + (sizeof(struct kevent) * max_files));
}

template<typename FileBufExt>
inline void file_bulk_reader<FileBufExt>::push_file(const char* file_path) {
    _PROFILER_AUTO();

    int fd = open(file_path, O_RDONLY | O_NONBLOCK);
    auto events = ((struct kevent*)this->events);

    // Register file descriptor with kqueue for read events
    EV_SET(&events[this->num_files], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

    if (kevent(this->kq, &events[this->num_files], 1, NULL, 0, &file_bulk_reader_push_file_timeout) == -1) {
        perror("file_bulk_reader_push_file kevent");
        exit(EXIT_FAILURE);
    }

    ++this->num_files;
}

template<typename FileBufExt>
NO_INLINE void file_bulk_reader<FileBufExt>::wait_open_all() {
    _PROFILER_AUTO();

    auto events = ((struct kevent*)this->events);

    for (auto it = events, events_end = events + this->num_files; it != events_end; ++it) {
        it->flags |= EV_ONESHOT;
    }

    while (1) {
        // Wait for events
        int nevents = kevent(this->kq, NULL, 0, events, this->num_files, &file_bulk_reader_push_file_timeout);
        if (nevents == -1) {
            KS_CRIT_LOG_ERROR("kevent returned -1");
        }

        if (nevents == 0) {
            break;
        }
    }
}


template<typename FileBufExt>
NO_INLINE void file_bulk_reader<FileBufExt>::wait_open_read_all() {
    _PROFILER_AUTO();

    auto events = ((struct kevent*)this->events);

    while (1) {
        // Wait for events
        int nevents = kevent(this->kq, NULL, 0, events, this->num_files, &file_bulk_reader_push_file_timeout);
        if (nevents == -1) {
            KS_CRIT_LOG_ERROR("kevent returned -1");
        }
    
        // Process events
        for (int i = 0; i < nevents; i++) {
            if (events[i].flags & EV_EOF) {
                // close(events[i].ident);
                EV_SET(&events[i], events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                if (kevent(this->kq, &events[i], 1, NULL, 0, &file_bulk_reader_push_file_timeout) == -1) {
                    KS_CRIT_LOG_ERROR("kevent returned -1");
                }
            } else
            // Check for read event
            if (events[i].filter == EVFILT_READ) {
                auto& rf_buf = this->file_bufs[i];

                if (rf_buf.size == 0) {
                    struct stat stat;
                    if (fstat((int)events[i].ident, &stat) != 0) {
                        KS_CRIT_LOG_ERROR("failed fstat", events[i].ident);
                    }
                    rf_buf.size = (size_t)stat.st_size;
                    char* buf = rf_buf.buf = new char[stat.st_size];
                    rf_buf.buf_read_it = buf;

                    // KS_LOG("fstat ", events[i].ident, stat.st_size, stat.st_blksize);
                }

                // KS_LOG("Data available on ", events[i].ident, rf_buf.size);

                while(1) {
                    int nread = read(events[i].ident, rf_buf.buf_read_it, _file_bulk_reader_CHUNK_SIZE);
                    if (nread > 0) {
                        rf_buf.buf_read_it += nread;
                    } else {
                        break;
                    }
                }
            }
        }

        if (nevents == 0) {
            break;
        }
    }
}


template<typename FileBufExt>
inline void file_bulk_reader<FileBufExt>::close_fds() {
    _PROFILER_AUTO();

    auto events = ((struct kevent*)this->events);

    for (int i = 0; i < this->num_files; ++i) {
        close(events[i].ident);
    }
}

template<typename FileBufExt>
inline void file_bulk_reader<FileBufExt>::free() {
    _PROFILER_AUTO();

    // frees all coz we allocate events & buffer as single mem chunk
    free(this->events);
}

#endif