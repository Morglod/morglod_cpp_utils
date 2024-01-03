// #pragma once

// #include "file_repository.hpp"
// #include "robin_hood.h"
// #include "./profiler.hpp"

// #include <cstring>
// #include <filesystem>
// #include <zlib.h>
// #include <libtar.h>
// #include <fcntl.h>

// #include "./libtar_ext.hpp"

// std::vector<gzFile> _gztar_opened;

// inline gzFile gztartype_get_file(int h) {
//     return _gztar_opened[h];
// }

// int gztartype_open(const char* pathname, int flags, mode_t mode) {
//     _PROFILER_AUTO();

//     char gz_flags[3] = "rb";

//     switch ( flags & O_ACCMODE ) {
// 	case O_WRONLY:
// 		gz_flags[0] = 'w';
// 		break;
// 	case O_RDONLY:
// 		gz_flags[0] = 'r';
// 		break;
// 	default:
// 	case O_RDWR:
// 		errno = EINVAL;
// 		return -1;
// 	}

//     auto fd = open(pathname, flags, mode);
//     gzFile gzfd = gzdopen(fd, gz_flags);
//     if (gzfd == NULL) {
//         close(fd);
//         printf("gztartype_open failed gzdopen\n");
//         return -1;
//     }

//     _gztar_opened.emplace_back(gzfd);

//     return _gztar_opened.size() - 1;
// }

// int gztartype_close(int h) {
//     _PROFILER_AUTO();

//     gzclose(gztartype_get_file(h));
//     _gztar_opened[h] = nullptr;
//     while(_gztar_opened.back() == nullptr) _gztar_opened.pop_back();
//     return 0;
// }

// ssize_t gztartype_read(int h, void * buf, size_t sz) {
//     _PROFILER_AUTO();

//     auto f = gztartype_get_file(h);
//     auto r = gzread(f, buf, sz);

//     if (r < 0) {
//         int fs_err = 0;
//         auto errstr = gzerror(f, &fs_err);
//         if (fs_err) {
//             printf("gztartype_read fserr: %i\n", fs_err);
//         }
//         if (errstr) {
//             printf("gztartype_read err: %s\n", errstr);
//         }
//         gzclearerr(f);
//     }

//     return r;
// }

// ssize_t gztartype_write(int h, const void * buf, size_t sz) {
//     return gzwrite(gztartype_get_file(h), buf, sz);
// }

// tartype_t gz_tartype = tartype_t {
// 	.openfunc = (openfunc_t)(void*)gztartype_open,
// 	.closefunc = gztartype_close,
// 	.readfunc = gztartype_read,
// 	.writefunc = gztartype_write
// };

// void tar_extract2(const char* tar_src_filename, const char* target_dir, int substr_archive_entry_path) {
//     _PROFILER_AUTO();

//     TAR* tar;
//     int th_found, ret;
//     ret = tar_open(&tar, tar_src_filename, &gz_tartype, O_RDONLY, 0, 0);
//     if (ret != 0) {
//         printf("failed tar_open %i", ret);
//         return;
//     }

//     char tmpbuf[512];

//     char dst_file_path[512]; // TODO: assert for tarentry.filename length
//     dst_file_path[0] = '\0';
//     strcpy(dst_file_path, target_dir);
//     char* dst_file_path_subpath = &dst_file_path[strlen(dst_file_path)];

//     // int i = 0;
//     // char buf[512];
//     // int size = 0;

//     while ((ret = th_read_ext(tar)) == 0) {
//         char *filename = th_get_pathname(tar);

//         // if (TH_ISREG(tar) && (tar_skip_regfile(tar) != 0)) {
//         //     fprintf(stderr, "tar_skip_regfile()\n");
//         //     break;
//         // }

//         strcpy(dst_file_path_subpath, &filename[substr_archive_entry_path]);
//         if (tar_extract_file2(tar, dst_file_path, tmpbuf) != 0) {
//             printf("extract failed \"%s\"\n", dst_file_path);
//         }

//         // std::filesystem::create_directories(std::filesystem::path(dst_file_path).parent_path());
//         // int wfd = open(dst_file_path, O_CREAT | O_WRONLY | O_TRUNC);

//         // for (i = size; i > 0; i -= T_BLOCKSIZE) {
//         //     int n_buf = tar_block_read(tar, buf);
//         //     if (n_buf == EOF) {
//         //         break;
//         //     }

//         //     write(wfd, buf, (n_buf > T_BLOCKSIZE)? T_BLOCKSIZE : n_buf);
//         // }
//         // close(wfd);
//     }

//     if (ret < 0) {
//         printf("th_read failed %i\n", errno);
//     }

//     ret = tar_close(tar);
//     tar = NULL;
// }