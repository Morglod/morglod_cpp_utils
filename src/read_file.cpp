#include "./read_file.hpp"

#include "./log.hpp"
#include "./profiler.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// void read_map_file(std::string const& file_path, std::string& out) {
//     // https://stackoverflow.com/questions/17925051/fast-textfile-reading-in-c
//     // !! mapping file to mem is slower !!
// }

void read_file_all(const char* c_str_file_path, char** out_buf, uint32_t* out_size) {
    // https://stackoverflow.com/questions/17925051/fast-textfile-reading-in-c

    _PROFILER_AUTO();

    int fd = open(c_str_file_path, O_RDONLY);
    if(fd == -1);

    struct stat stat;
    fstat(fd, &stat);

    if (stat.st_size == 0) {
        KS_CRIT_LOG_ERROR("failed read file size=0", c_str_file_path);
        return;
    }
    
    // const uint32_t BUFFER_SIZE = stat.st_size;

    char* buf = new char[stat.st_size];
    // uint32_t buf_size = BUFFER_SIZE;
    uint32_t buf_len = 0;

    /* Advise the kernel of our access pattern.  */
    // posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    // no buffer variant
    {
        while(size_t bytes_read = read(fd, buf + buf_len, stat.st_size))
        {
            if(bytes_read == (size_t)-1);
            if (!bytes_read)
                break;

            // if (buf_len + (BUFFER_SIZE / 2) >= buf_size) {
            //     // realloc
            //     const auto new_size = buf_size + buf_size / 2;
            //     char* new_buf = (char*)realloc(buf, new_size);
            //     if (new_buf != buf) {
            //         memcpy(new_buf, buf, buf_len);
            //     }
            //     buf_size = new_size;
            //     buf = new_buf;
            // }
            buf_len += bytes_read;
        }
    }

    // with buffer variant
    // {
    //     char tmp_buf[BUFFER_SIZE + 1];
    //     while(size_t bytes_read = read(fd, tmp_buf, BUFFER_SIZE))
    //     {
    //         if(bytes_read == (size_t)-1);
    //         if (!bytes_read)
    //             break;

    //         if (buf_len + bytes_read >= buf_size) {
    //             const auto new_size = buf_size + buf_size / 2;
    //             char* new_buf = (char*)realloc(buf, new_size);
    //             if (new_buf != buf) {
    //                 memcpy(new_buf, buf, buf_len);
    //             }
    //             buf_size = new_size;
    //             buf = new_buf;
    //         }
    //         memcpy(buf + buf_len, tmp_buf, bytes_read);
    //         buf_len += bytes_read;
    //     }
    // }

    close(fd);

    *out_buf = buf;
    *out_size = buf_len;
}
