#pragma once

#include "./macro_map.hpp"

#include <string>

#define _EXPAND_ENUM(x) x,
#define _EXPAND_ENUM_TO_STR(x) #x ,

#define ENUM_STR(_NAME, _TYPE, ...) \
    enum _NAME : _TYPE { \
        MACRO_MAP(_EXPAND_ENUM, __VA_ARGS__) \
        _NAME##__MAX_SIZE_ \
    }; \
    namespace { \
    inline const std::string __##_NAME##_TO_STRING[_NAME##__MAX_SIZE_] = { \
        MACRO_MAP(_EXPAND_ENUM_TO_STR, __VA_ARGS__) \
    }; \
    } \
    inline std::string const& _NAME##_to_string(_NAME e) { \
        return __##_NAME##_TO_STRING[e]; \
    } \
    inline _NAME _NAME##_from_string(std::string const s) { \
        for (_TYPE i = 0; i < _NAME##__MAX_SIZE_; ++i) { \
            if (__##_NAME##_TO_STRING[i] == s) return (_NAME)i; \
        } \
        throw "failed " #_NAME " from string"; \
    }
