#pragma once

#include <type_traits>
#include <vector>
#include <atomic>
#include <mutex>

// circullar lock free buffer with over_limit vector
template<typename T, int LIMIT>
class LockFreeBuf {
public:
    std::atomic<int> write_pos;
    std::atomic<int> read_pos;
    T* buffer;

    std::atomic<bool> over_limit_flag;
    std::mutex over_limit_mtx;
    std::vector<T> over_limit;

    inline bool has_smth_to_read() const {
        int rp = read_pos.load(), wp = write_pos.load();
        return rp != wp || over_limit_flag.load();
    }

    void push(T const& item) {
        int rp = read_pos.load(), wp = write_pos.load();

        if (wp + 1 >= LIMIT) {
            wp = -1;
        }

        if (wp + 1 == rp) {
            std::lock_guard lock(over_limit_mtx);
            over_limit_flag = true;
            over_limit.emplace_back(item);
        } else {
            buffer[wp + 1] = item;
            write_pos = wp + 1;
        }
    }

    std::vector<T> read() {
        std::vector<T> result;

        int rp = read_pos.load(), wp = write_pos.load();
        if (wp < rp) {
            result.reserve(LIMIT + wp - 1);
            for (int i = rp; i < LIMIT; ++i) {
                result.emplace_back(buffer[i]);
            }
            for (int i = 0; i < wp; ++i) {
                result.emplace_back(buffer[i]);
            }
        } else {
            result.reserve(wp - rp + 1);
            for (int i = rp; i <= wp; ++i) {
                result.emplace_back(buffer[i]);
            }
        }
        if (over_limit_flag.load()) {
            std::lock_guard lock(over_limit_mtx);
            for (int i = 0, len = (int)over_limit.size(); i < len; ++i) {
                result.emplace_back(over_limit[i]);
            }
            over_limit.clear();
            over_limit_flag = false;
        }
        read_pos = wp;

        return result;
    }

    LockFreeBuf() : write_pos(0), read_pos(0), buffer(new T[LIMIT]) {
        over_limit.reserve(10);
    }
    ~LockFreeBuf() {
        delete [] buffer;
    }
};

// lock free double buffer (cache friendly) with over_limit vector
// template<typename T, int LIMIT, int BUFFERS_NUM = 2, typename = std::is_trivially_copyable<T>>
// class LockFreeBufQueue {
// protected:
//     // should be aligned to split on cachelines for concurrent access even to buf pointer
//     struct alignas(64) Buffer {
//         T* buf;
//         // was written to this buf and not readed
//         std::atomic<int> written;
//     };

// public:

//     // inline bool has_smth_to_read() const {
//     //     int rp = read_pos.load(), wp = write_pos.load();
//     //     return rp != wp || over_limit_flag.load();
//     // }

//     void push(T const& item) {
//         const auto wb = write_buf.load();
//         const auto rb = read_buf.load();
//         if (wb == rb) {
//             std::lock_guard lock(over_limit_mtx);
//             over_limit.emplace_back(item);
//             return;;
//         }

//         auto& buf = buffers[wb];
//         auto wp = buf.written;
//         buf.buf[wp] = item;
//         buf.written = ++wp;

//         if (wp >= LIMIT) {
//             ++write_buf;
//         }
//     }
    
//     std::vector<T> read_some() {
//         std::vector<T> result;
//         auto rb = read_buf.load();

//         auto& buf = buffers[rb];
//     }

//     LockFreeBufQueue() : write_buf(1), read_buf(0) {
//         for (int i = 0; i < BUFFERS_NUM; ++i) {
//             buffers[i] = new T[LIMIT];
//         }
//     }

//     ~LockFreeBufQueue() {
//         for (int i = 0; i < BUFFERS_NUM; ++i) {
//             delete [] buffers[i];
//         }
//     }

// protected:
//     std::atomic<uint8_t> write_buf;
//     std::atomic<uint8_t> read_buf;

//     // TODO: benchmark; coz accessing buffer[i] we still reading `buffers` variable (which is pointer) to do buffers+i to access buffers[i]
//     // so may be better do not allign this `buffers` array and use memcpy to access specific buffer without cache invalidation
//     /* alignas(64) actually we shouldnt get cache miss here, coz we write only to aligned Buffer's mem (when changing BUffer:written) */ Buffer buffers[BUFFERS_NUM];

//     std::mutex over_limit_mtx;
//     std::vector<T> over_limit;
// };