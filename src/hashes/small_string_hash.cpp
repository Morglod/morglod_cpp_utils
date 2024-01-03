#include "./small_string_hash.hpp"
#include <ctime>

uint64_t _init_small_string_hash_wyp[4];

void init_small_string_hash() {
    wyhash_make_secret(time(0), _init_small_string_hash_wyp);
}

uint64_t small_string_hash(const void* key, size_t len) {
    return wyhash(key, len, 0, _init_small_string_hash_wyp);
}