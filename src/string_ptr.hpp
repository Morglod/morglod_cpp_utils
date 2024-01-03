#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>

#include "./hashes/small_string_hash.hpp"
#include "./log.hpp"
#include "./memory.hpp"
#include "./strstr_optimized.hpp"

#include <fmt/core.h>

// no deletion
struct string_ptr {
    typedef uint64_t hash_type;

    const char* _data = nullptr;
    hash_type _hash = 0;
    uint32_t _size = 0;

    inline hash_type hash() const { return _hash; }
    inline const char* data() { return _data; }
    inline const char* cbegin() const { return _data; }
    inline const char* cend() const { return _data + _size; }
    inline uint32_t size() const { return _size; }

    inline string_ptr(const char* data, uint32_t size) : _data(data), _hash(small_string_hash(data, size)), _size(size) {}

    inline bool operator == (string_ptr const& other) const {
        return _hash == other._hash;
    }

    inline char operator [] (uint32_t i) const { return _data[i]; }

    inline std::string to_std() const {
        std::string str;
        str.resize(size());
        str.assign(_data, size());
        return str;
    }

    inline std::string_view to_string_view() const {
        return std::string_view(cbegin(), size());
    }

    template<size_t LEN>
    inline bool equal(const char (&str)[LEN]) const {
        // LEN-1 coz string_ptr dont handle \0
        return (size() == LEN - 1) && precompile_memcmp<LEN - 1>(_data, str);
    }

    inline bool equal(const char* str, uint32_t len) const {
        return switch_memcmp(_data, str, len) == 0;
    }

    string_ptr() = default;
    inline string_ptr(string_ptr const& other) : _data(other._data), _hash(other._hash), _size(other._size) {}
};

struct string_owner {
    string_ptr str;

    inline void copy_cstr(const char* s, uint32_t size) { 
        dispose();
        char* buf = new char[size];
        switch_memcpy(buf, s, size);
        str = std::move(string_ptr(s, size));
    }

    inline void copy_string(std::string& s) { 
        dispose();
        char* buf = new char[s.size()];
        switch_memcpy(buf, s.data(), s.size());
        str = std::move(string_ptr(buf, s.size()));
    }

    inline void copy_string(string_ptr const& s) { 
        dispose();
        char* buf = new char[s.size()];
        switch_memcpy(buf, s.cbegin(), s.size());
        str = std::move(string_ptr(buf, s.size()));
    }

    inline void dispose() {
        if (str._data) {
            delete [] str._data;
            str._data = nullptr;
        }
    }

    string_owner() = default;
    inline ~string_owner() {
        dispose();
    }
};

namespace std {
    template <>
    struct std::hash<string_ptr> {
        inline size_t operator()(string_ptr const & x) const {
            // std::hash<std::string> h;
            // return h(x.to_std());
            return (size_t)x._hash;
        }
    };
}

#ifdef FMT_CORE_H_

namespace fmt {
    template <>
    struct formatter<string_ptr> {
        constexpr auto parse(format_parse_context& ctx) {
            return std::end(ctx);
        }

        template <typename FormatContext>
        auto format(string_ptr const& v, FormatContext& ctx)  {
            auto&& out = ctx.out();
            for (int i = 0; i < v.size(); ++i)
                format_to(out, "{}", v[i]);
            return format_to(out, "");
        }
    };
}

#endif