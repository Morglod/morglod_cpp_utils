#pragma once

#define USE_WYHASH

#ifdef USE_WYHASH

#include "./deps/wyhash/wyhash.hpp"

void init_small_string_hash();
uint64_t small_string_hash(const void* key, size_t len);

// NOT WORKING
// inline uint32_t small_string_hash_half(const void* key, size_t len) {
//     return (uint32_t)wyhash(key, len, 0, _init_small_string_hash_wyp);
// }

#else

#endif
