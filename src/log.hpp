#pragma once

#include "./macro_map.hpp"

#include <fmt/printf.h>
#include <fmt/color.h>
#include <fmt/chrono.h>
#include <string_view>

#if _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#else
    // #define __PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#endif

#define _KS_LOG_INSERTION_POINT_(x) " {}"

inline std::tm _KS_LOG_time_now() {
    auto time = std::time(nullptr);
    return *std::gmtime(&time);
}

#define _KS_LOG_FMT_ARGS(_WHERE, ...) \
    "{:%H:%M:%S} {} |" \
        MACRO_MAP(_KS_LOG_INSERTION_POINT_, __VA_ARGS__) \
        "\n" \
    , _KS_LOG_time_now(), _WHERE, __VA_ARGS__

#if _MSC_VER && !__INTEL_COMPILER
    constexpr std::string _ks_log_cleanup_function_name(std::string const& pretty_name, std::string short_name) {
        short_name += "(";
        size_t it = pretty_name.find(short_name);
        size_t found = 0;
        for (size_t i = it - 1; i >= 0; --i) {
            if (pretty_name[i] == ' ') {
                found = i;
                break;
            }
        }
        return pretty_name.substr(found, it + short_name.size() - 1 - found);
    }

    #define _KS_LOG_CLEAN_FUNC_NAME _ks_log_cleanup_function_name(__PRETTY_FUNCTION__, __func__)
#else
    constexpr std::string_view _ks_log_cleanup_function_name(std::string_view pretty_name) {
        int from = pretty_name.find_first_of(' ') + 1;
        int to = pretty_name.find_first_of('(', from);
        return pretty_name.substr(from, to - from);
    }

    #define _KS_LOG_CLEAN_FUNC_NAME _ks_log_cleanup_function_name(__PRETTY_FUNCTION__)
#endif

#define KS_LOG(...) fmt::print(_KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, __VA_ARGS__));
#define KS_CRIT_LOG(...) fmt::print(stdout, _KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, __VA_ARGS__));

#define KS_FORMAT_ARGS_ERROR(...) _KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("ERROR", fmt::fg(fmt::color::crimson)), __VA_ARGS__)
#define KS_LOG_ERROR(...) fmt::print(_KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("ERROR", fmt::fg(fmt::color::crimson)), __VA_ARGS__));
#define KS_CRIT_LOG_ERROR(...) fmt::print(stdout, _KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("ERROR", fmt::fg(fmt::color::crimson)), __VA_ARGS__));

#define KS_LOG_WARN(...) fmt::print(_KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("WARN", fmt::fg(fmt::color::orchid)), __VA_ARGS__));
#define KS_CRIT_LOG_WARN(...) fmt::print(stdout, _KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("WARN", fmt::fg(fmt::color::orchid)), __VA_ARGS__));

#define KS_LOG_SUCCESS(...) fmt::print(_KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("SUCCESS", fmt::fg(fmt::color::lawn_green)), __VA_ARGS__));
#define KS_CRIT_LOG_SUCCESS(...) fmt::print(stdout, _KS_LOG_FMT_ARGS(_KS_LOG_CLEAN_FUNC_NAME, fmt::styled("SUCCESS", fmt::fg(fmt::color::lawn_green)), __VA_ARGS__));

// #define KS_LOG_DEBUG(...) KS_CRIT_LOG(__VA_ARGS__)
#define KS_LOG_DEBUG(...) {};

#define KS_EXCEPTION(...) \
    { \
        const auto str = fmt::format(KS_FORMAT_ARGS_ERROR(__VA_ARGS__)); \
        KS_CRIT_LOG_ERROR(str); \
        throw std::runtime_error(str.c_str()); \
    }

#define KS_IF_EXCEPTION( IF_EXPR, ... ) if ( IF_EXPR ) KS_EXCEPTION(__VA_ARGS__);