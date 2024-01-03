#pragma once

#include "./macro_map.hpp"

#define _DECL_FLAG_SET_X(_X) _FLAG_##_X,

#define _DECL_FLAG_SET_DEBUG_PRINT_ITEM(_X) \
    if (get_##_X()) out << #_X << ", ";

#define _DECL_FLAG_SET_ITEM(_X) \
    inline void set_##_X(bool x) { \
        if (x) flags |= (1 << _FLAG_##_X); \
        else flags = flags & (~ (1 << _FLAG_##_X)); \
    } \
    inline bool has_##_X() const { \
        return flags & (1 << _FLAG_##_X); \
    }

#define DECL_FLAG_SET(_NAME, _BASE_TYPE, ...) \
    struct _NAME { \
        _BASE_TYPE flags = 0; \
        enum : _BASE_TYPE { \
            _NONE_ = 0, \
            MACRO_MAP(_DECL_FLAG_SET_X, __VA_ARGS__) \
            _BITS_NUM_ \
        }; \
        MACRO_MAP(_DECL_FLAG_SET_ITEM, __VA_ARGS__) \
        inline void debug_print(std::ostream& out) const { \
            out << "["; \
            MACRO_MAP(_DECL_FLAG_SET_DEBUG_PRINT_ITEM, __VA_ARGS__) \
            out << "]"; \
        } \
    }

#define _DECL_FLAG_SET_VARIANT_ITEM(_X) \
    inline void set_##_X(_X&& x) { \
        variant = std::move(x); \
    } \
    inline bool get_##_X(_X*& out) { \
        if (variant.index() != (size_t)_FLAG_##_X) { \
            return false; \
        } \
        _X& val = std::get<_X>(variant); \
        out = &val; \
        return true; \
    } \
    inline bool is_##_X() { \
        if (variant.index() != (size_t)_FLAG_##_X) { \
            return false; \
        } \
        return true; \
    }

#define DECL_FLAG_SET_VARIANT_INLINE(_NAME, ...) \
    struct { \
        enum { \
            _NONE_ = 0, \
            MACRO_MAP(_DECL_FLAG_SET_X, __VA_ARGS__) \
            _BITS_NUM_ \
        }; \
        std::variant<__VA_ARGS__> variant; \
        MACRO_MAP(_DECL_FLAG_SET_VARIANT_ITEM, __VA_ARGS__) \
        inline void debug_print(std::ostream& out) const { \
            out << "["; \
            MACRO_MAP(_DECL_FLAG_SET_DEBUG_PRINT_ITEM, __VA_ARGS__) \
            out << "]"; \
        } \
    } _NAME;
