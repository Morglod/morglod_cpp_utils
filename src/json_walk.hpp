#pragma once

#include "./_globals.hpp"
#include "./memory.hpp"
#include "./profiler.hpp"
#include "./log.hpp"

#include <cstring>
#include <string>

#include "./string_ptr.hpp"

#include "./strstr_optimized.hpp"

#define _JSON_WALK_PROFILER_USE 0

#ifndef _JSON_WALK_PROFILER_USE
#define _JSON_WALK_PROFILER_USE 0
#endif

#if _JSON_WALK_PROFILER_USE == 1
    #define _JSON_WALK_PROFILER_AUTO(...) _PROFILER_AUTO(__VA_ARGS__)
    #define _JSON_WALK_PROFILER_START(...) _PROFILER_START(__VA_ARGS__)
    #define _JSON_WALK_PROFILER_END(...) _PROFILER_END(__VA_ARGS__)
#else
    #define _JSON_WALK_PROFILER_AUTO(...) {};
    #define _JSON_WALK_PROFILER_START(...) {};
    #define _JSON_WALK_PROFILER_END(...) {};
#endif

// string_start should point to  "some string"
//                              ^
inline const char* json_util_skip_string(const char* string_start, const char* limit) {
    ++string_start;
    const auto found = strstr_opt(string_start, limit - string_start, "\"", 1);
    if (found != std::string::npos) return string_start + found + 1;
    return nullptr;
}

inline const char* json_util_skip_until(const char* start, const char* limit, char delim) {
    _JSON_WALK_PROFILER_AUTO();

    auto r = strstr_opt(start, limit - start, &delim, 1);
    if (r == std::string::npos) return nullptr;
    KS_LOG_DEBUG("json_util_skip_until strstr_opt", r, limit - start);
    return start + r;
}

inline const char* json_util_next_start_delimeter(const char* start, const char* limit) {
    _JSON_WALK_PROFILER_AUTO();

    for (const char* ch = start; ch != limit; ++ch) {
        if (*ch == ':' || *ch == '"' || *ch == '{' || *ch == '[') {
            return ch;
        }
    }
    return nullptr;
}

inline const char* json_util_next_end_delimeter(const char* start, const char* limit) {
    _JSON_WALK_PROFILER_AUTO();

    for (const char* ch = start; ch != limit; ++ch) {
        if (*ch == ',' || *ch == '"' || *ch == '}' || *ch == ']') {
            return ch;
        }
    }
    return nullptr;
}

inline const char* json_util_skip_spaces(const char* start, const char* limit) {
    _JSON_WALK_PROFILER_AUTO();

    for (const char* ch = start; ch != limit; ++ch) {
        if (*ch == '\n') continue;
        if (*ch == '\r') continue;
        if (*ch != ' ') {
            return ch;
        }
    }
    return nullptr;
}

inline const char* json_util_find_part_with_depth(
    const char* start, const char* limit,
    const char* part_text, int part_text_size,
    int depth
) {
    _JSON_WALK_PROFILER_START("json_util_find_part_with_depth");

    char part_text_last_char = part_text[part_text_size - 1];
    int current_depth = 0;

    for (const char* ch = start; ch != limit; ++ch) {
        if (current_depth == -1) {
            _JSON_WALK_PROFILER_END("json_util_find_part_with_depth");
            return nullptr;
        }
        if (depth == current_depth && ch[0] == part_text[0] && ch[1] == part_text[1] && ch[part_text_size - 1] == part_text_last_char) {
            if (switch_memcmp(&ch[2], &part_text[2], part_text_size - 2) && (limit - ch >= part_text_size)) {
                _JSON_WALK_PROFILER_END("json_util_find_part_with_depth");
                return ch;
            }
        }
        if (*ch == '"') {
            ch = json_util_skip_string(ch, limit);
            if (!ch) {
                _JSON_WALK_PROFILER_END("json_util_find_part_with_depth");
                return nullptr;
            }
            --ch; // coz ++ch on next iteration
            continue;
        }
        if (*ch == '{') {
            ++current_depth;
            continue;
        }
        if (*ch == '}') {
            --current_depth;
            continue;
        }
        if (*ch == '[') {
            ++current_depth;
            continue;
        }
        if (*ch == ']') {
            --current_depth;
            continue;
        }
    }
    _JSON_WALK_PROFILER_END("json_util_find_part_with_depth");
    return nullptr;
}

// field_end should pint to  "field_name": {
//                                       ^
inline const char* json_util_enter_field(const char* field_end, const char* limit) {
    _JSON_WALK_PROFILER_AUTO();

    for (const char* ch = field_end; ch != limit; ++ch) {
        if (*ch == '{') {
            return ch + 1;
        }
    }

    return nullptr;
}

inline const char* json_util_next_field_same_depth(const char* start, const char* limit, const char** out_str_end) {
    _JSON_WALK_PROFILER_START("json_util_next_field_same_depth");

    int current_depth = 0;

    for (const char* ch = start; ch != limit; ++ch) {
        if (current_depth == -1) {
            _JSON_WALK_PROFILER_END("json_util_next_field_same_depth");
            return nullptr;
        }
        if (*ch == '"') {
            if (current_depth != 0) {
                ch = json_util_skip_string(ch, limit);
                --ch; // coz ++ch on next iteration
                continue;
            }

            auto str_end = json_util_skip_string(ch, limit);
            if (!str_end) {
                _JSON_WALK_PROFILER_END("json_util_next_field_same_depth");
                return nullptr;
            }
            auto delim = json_util_skip_spaces(str_end, limit);
            if (*delim == ':') {
                *out_str_end = str_end;
                _JSON_WALK_PROFILER_END("json_util_next_field_same_depth");
                return ch;
            }

            ch = str_end;

            continue;
        }
        if (*ch == ':') {
            ++ch;
            ch = json_util_skip_spaces(ch, limit);
            if (*ch == '"') ch = json_util_skip_string(ch, limit);
            else {
                if (*ch >= '0' && *ch <= '9') {
                    ch = json_util_next_end_delimeter(ch, limit);
                }
            }
        }
        if (*ch == '[') {
            ++current_depth;
            continue;
        }
        if (*ch == ']') {
            --current_depth;
            continue;
        }
        if (*ch == '{') {
            ++current_depth;
            continue;
        }
        if (*ch == '}') {
            --current_depth;
            continue;
        }
    }

    _JSON_WALK_PROFILER_END("json_util_next_field_same_depth");
    return nullptr;
}


// field_end shoud point to "field": "value", ..
//                                 ~^
inline string_ptr json_util_string_value(const char* from, const char* limit) {
    _JSON_WALK_PROFILER_AUTO();

    from = json_util_skip_until(from, limit, '"');
    auto str_end = json_util_skip_string(from, limit);
    return string_ptr(from + 1, str_end - from - 2);
}

// start shoud point to "field": {  "subfield1", ..
//                                 ^
// Callback(string_ptr&& field_name, const char* field_end)
template<typename Callback>
void json_util_pick_field_names(const char* start, const char* limit, Callback callback) {
    _JSON_WALK_PROFILER_AUTO();

    do {
        const char* str_end = nullptr;
        start = json_util_next_field_same_depth(start, limit, &str_end);
        if (!start) {
            return;
        }
        callback(std::move(string_ptr(start + 1, str_end - start - 2)), str_end);
        start = str_end;
    } while(1);
}