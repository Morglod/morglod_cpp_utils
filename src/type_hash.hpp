#pragma once

#include "./crc32.hpp"

template<typename T>
inline constexpr size_t type_hash() noexcept {

#if _MSC_VER
    return (size_t)(uint_crc32(__FUNCSIG__));
#else
    return (size_t)(uint_crc32(__PRETTY_FUNCTION__));
#endif

}