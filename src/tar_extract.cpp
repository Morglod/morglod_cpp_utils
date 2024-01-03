#include "./tar_extract.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <filesystem>

#include "./fs_create_dir.hpp"
#include "./fs_preallocate_file.hpp"
#include "./log.hpp"
#include "./profiler.hpp"
#include "./read_file.hpp"

// !!!!!!!!!!!!!!!!!!

// https://github.com/libarchive/libarchive/wiki/Examples#user-content-A_Complete_Extractor

namespace {

int copy_data(struct archive *ar, struct archive *aw);
void errmsg(const char *m);
void warn(const char *f, const char *m);

}

void tar_extract(const char *filename, char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path)
{
    _PROFILER_AUTO();

    constexpr size_t read_block_size = 1024 * 1024;

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;
    int r;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;
    flags |= ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    if ((r = archive_read_open_filename(a, filename, read_block_size)))
        throw std::runtime_error("failed read archive " + std::string(filename));
    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(a));
        if (r < ARCHIVE_WARN)
            throw std::runtime_error("failed read status for " + std::string(filename) + " " + std::to_string(r));
    
        // sprintf(_tar_extract_filepath_buf, "%s/%s", dst_path, &archive_entry_pathname(entry)[substr_archive_entry_path]);
        strcpy(&filepath_buf[dst_path_len], &archive_entry_pathname(entry)[substr_archive_entry_path]);
        // TODO use fs_preallocate_file
        // https://github.com/oven-sh/bun/blob/main/src/libarchive/libarchive.zig#L611
        archive_entry_set_pathname(entry, filepath_buf);

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(ext));
        else if (archive_entry_size(entry) > 0) {
            r = copy_data(a, ext);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(ext));
            if (r < ARCHIVE_WARN)
                throw std::runtime_error("failed write status for " + std::string(filename) + " " + std::to_string(r)); //  + " dest_file=" + std::string(filepath_buf));
        }
        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(ext));
        if (r < ARCHIVE_WARN)
            throw std::runtime_error("failed write finish status for " + std::string(filename) + " " + std::to_string(r));
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
}

namespace {

int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
	int64_t offset;
#else
	off_t offset;
#endif

	for (;;) {
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r != ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(aw, buff, size, offset);
		if (r != ARCHIVE_OK) {
			warn("archive_write_data_block()",
			    archive_error_string(aw));
			return (r);
		}
	}
}

void errmsg(const char *m)
{
	write(2, m, strlen(m));
}

void warn(const char *f, const char *m)
{
	errmsg(f);
	errmsg(" failed: ");
	errmsg(m);
	errmsg("\n");
}

}


void tar_extract_tgz_small_from_mem(
    const char* file_data, size_t file_size,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path
)
{
    _PROFILER_AUTO();

    struct archive *a;
    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_format_gnutar(a);
    // archive_read_support_compression_gzip(a); // use *_filter_gzip instead??
    archive_read_support_filter_gzip(a);
    archive_read_set_options(a, "read_concatenated_archives");
    archive_read_open_memory(a, file_data, file_size);

    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF){
            break;
        }
        if (r < ARCHIVE_OK) {
            KS_CRIT_LOG_ERROR("archive_read_next_header failed", archive_error_string(a));
            return;
        }
        if (r < ARCHIVE_WARN) {
            throw std::runtime_error("failed read status " + std::to_string(r));
        }
    
        // sprintf(_tar_extract_filepath_buf, "%s/%s", dst_path, &archive_entry_pathname(entry)[substr_archive_entry_path]);
        strcpy(&filepath_buf[dst_path_len], &archive_entry_pathname(entry)[substr_archive_entry_path]);
        // TODO use fs_preallocate_file
        // https://github.com/oven-sh/bun/blob/main/src/libarchive/libarchive.zig#L611
        // archive_entry_set_pathname(entry, filepath_buf);

        auto entry_file_type = archive_entry_filetype(entry);

        // directory
        if (entry_file_type == AE_IFDIR) {
            // https://github.com/oven-sh/bun/blob/main/src/libarchive/libarchive.zig#L533
            auto perm = archive_entry_perm(entry);

            // if dirs are readable, then they should be listable
            // https://github.com/npm/node-tar/blob/main/lib/mode-fix.js
            if ((perm & ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES) != 0)
                perm |= ARCHIVE_ENTRY_ACL_DELETE_CHILD;
            if ((perm & ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS) != 0)
                perm |= ARCHIVE_ENTRY_ACL_ADD_FILE | ARCHIVE_ENTRY_ACL_WRITE_DATA;
            if ((perm & ARCHIVE_ENTRY_ACL_READ) != 0)
                perm |= ARCHIVE_ENTRY_ACL_EXECUTE;
            
            // TODO: no strings optimization
            if (!fs_create_dir(filepath_buf)) {
                KS_CRIT_LOG_ERROR("failed create directories",std::filesystem::path(filepath_buf).parent_path().c_str());
            }
            continue;
        }

        // file
        if (entry_file_type == AE_IFREG) {
            _PROFILER_AUTO(tar_create_file);

            const auto entry_size = archive_entry_size(entry);
            if (entry_size == 0) {
                continue;
            }

            // struct stat _stat;
            // stat(filepath_buf, &_stat);
            // if (_stat.st_size == entry_size) {
            //     // skip
            //     continue;
            // } else {
            //     KS_LOG("diff size", filepath_buf, _stat.st_size, entry_size);
            // }

            auto perm = archive_entry_perm(entry);
            constexpr int open_flags = O_CREAT | O_TRUNC | O_WRONLY
                #ifdef __linux__
                    | O_DIRECT
                #endif
            ;
            auto fd = open(filepath_buf, open_flags, S_IWUSR | perm);
            if (fd == -1) {
                fs_create_dir(filepath_buf);
                fd = open(filepath_buf, open_flags, S_IWUSR | perm);
                if (fd == -1) {
                    KS_CRIT_LOG_ERROR("failed open file", filepath_buf);
                    return;
                }
            }

            #ifdef __APPLE__
            fcntl(fd, F_NOCACHE, 1);
            #endif

            if (entry_size > 4096) {
                fs_preallocate_file(fd, 0, entry_size);
            }

            _PROFILER_START("tar archive_read_data_into_fd", 3);
            int r = archive_read_data_into_fd(a, fd);
            _PROFILER_END("tar archive_read_data_into_fd", 3);
            close(fd);

            if (r == ARCHIVE_EOF) break;
            if (r != ARCHIVE_OK) {
                KS_CRIT_LOG_ERROR("archive_read_data_into_fd failed", archive_error_string(a));
            }

            continue;
        }

        // symlink
        if (entry_file_type == AE_IFLNK) {
            KS_CRIT_LOG_WARN("skip link");
            continue;
        }

        KS_CRIT_LOG_WARN("unknown file type", entry_file_type);
    }
    archive_read_close(a);
    archive_read_free(a);
}

void tar_extract_tgz_small_from_file(
    const char *filename,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path
)
{
    _PROFILER_AUTO();

    char* file_data;
    uint32_t file_data_size;
    read_file_all(filename, &file_data, &file_data_size);

    tar_extract_tgz_small_from_mem(file_data, file_data_size, filepath_buf, dst_path_len, substr_archive_entry_path);
}

void tar_extract_nodeflate_from_memory(
    const char* file_data, size_t file_size,
    char filepath_buf[1024], int dst_path_len, int substr_archive_entry_path
) {
    _PROFILER_AUTO();

    struct archive *a;
    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_format_gnutar(a);
    archive_read_set_options(a, "read_concatenated_archives");
    archive_read_open_memory(a, file_data, file_size);

    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF){
            break;
        }
        if (r < ARCHIVE_OK) {
            KS_CRIT_LOG_ERROR("archive_read_next_header failed", archive_error_string(a));
            return;
        }
        if (r < ARCHIVE_WARN) {
            throw std::runtime_error("failed read status " + std::to_string(r));
        }
    
        // sprintf(_tar_extract_filepath_buf, "%s/%s", dst_path, &archive_entry_pathname(entry)[substr_archive_entry_path]);
        strcpy(&filepath_buf[dst_path_len], &archive_entry_pathname(entry)[substr_archive_entry_path]);
        // TODO use fs_preallocate_file
        // https://github.com/oven-sh/bun/blob/main/src/libarchive/libarchive.zig#L611
        // archive_entry_set_pathname(entry, filepath_buf);

        auto entry_file_type = archive_entry_filetype(entry);

        // directory
        if (entry_file_type == AE_IFDIR) {
            // https://github.com/oven-sh/bun/blob/main/src/libarchive/libarchive.zig#L533
            auto perm = archive_entry_perm(entry);

            // if dirs are readable, then they should be listable
            // https://github.com/npm/node-tar/blob/main/lib/mode-fix.js
            if ((perm & ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES) != 0)
                perm |= ARCHIVE_ENTRY_ACL_DELETE_CHILD;
            if ((perm & ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS) != 0)
                perm |= ARCHIVE_ENTRY_ACL_ADD_FILE | ARCHIVE_ENTRY_ACL_WRITE_DATA;
            if ((perm & ARCHIVE_ENTRY_ACL_READ) != 0)
                perm |= ARCHIVE_ENTRY_ACL_EXECUTE;
            
            // TODO: no strings optimization
            if (!fs_create_dir(filepath_buf)) {
                KS_CRIT_LOG_ERROR("failed create directories",std::filesystem::path(filepath_buf).parent_path().c_str());
            }
            continue;
        }

        // file
        if (entry_file_type == AE_IFREG) {
            _PROFILER_START("tar create file", 2);
            auto perm = archive_entry_perm(entry);
            constexpr int open_flags = O_CREAT | O_TRUNC | O_WRONLY
                #ifdef __linux__
                    | O_DIRECT
                #endif
            ;
            auto fd = open(filepath_buf, open_flags, S_IWUSR | perm);
            if (fd == -1) {
                fs_create_dir(filepath_buf);
                fd = open(filepath_buf, open_flags, S_IWUSR | perm);
                if (fd == -1) {
                    KS_CRIT_LOG_ERROR("failed open file", filepath_buf);
                    _PROFILER_END("tar create file", 2);
                    return;
                }
            }

            #ifdef __APPLE__
            fcntl(fd, F_NOCACHE, 1);
            #endif

            const auto entry_size = archive_entry_size(entry);
            if (entry_size == 0) {
                // KS_CRIT_LOG_WARN("empty file, skipping", filepath_buf);
                close(fd);
                _PROFILER_END("tar create file", 2);
                continue;
            }

            if (entry_size > 4096) {
                fs_preallocate_file(fd, 0, entry_size);
            }
            _PROFILER_END("tar create file", 2);

            _PROFILER_START("tar archive_read_data_into_fd", 3);
            int r = archive_read_data_into_fd(a, fd);
            _PROFILER_END("tar archive_read_data_into_fd", 3);
            close(fd);

            if (r == ARCHIVE_EOF) break;
            if (r != ARCHIVE_OK) {
                KS_CRIT_LOG_ERROR("archive_read_data_into_fd failed", archive_error_string(a));
            }

            continue;
        }

        // symlink
        if (entry_file_type == AE_IFLNK) {
            KS_CRIT_LOG_WARN("skip link");
            continue;
        }

        KS_CRIT_LOG_WARN("unknown file type", entry_file_type);
    }
    archive_read_close(a);
    archive_read_free(a);
}