#pragma once

#include <functional>

template <class HashT, class T>
inline void hash_combine_value(HashT & s, const T & v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}

template <class HashT>
inline void hash_combine(HashT & s, HashT other_hash) {
    if (other_hash == 0) {
        throw std::runtime_error("cant calc zero hash");
    }
    s ^= other_hash + 0x9e3779b9 + (s<< 6) + (s>> 2);
}
