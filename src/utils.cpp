#include "./utils.hpp"

#include <chrono>
#include <algorithm>
#include <fmt/core.h>

void str_to_uppercase_inplace(std::string& str) {
    for (size_t i = 0, len = str.size(); i < len; ++i) str[i] = std::toupper(str[i]);
}

std::string str_to_uppercase(std::string const& from) {
    std::string to;
    to.resize(from.size());
    for (size_t i = 0, len = from.size(); i < len; ++i) to[i] = std::toupper(from[i]);
    return to;
}

std::string str_to_uppercase(std::string_view const& from) {
    std::string to;
    to.resize(from.size());
    for (size_t i = 0, len = from.size(); i < len; ++i) to[i] = std::toupper(from[i]);
    return to;
}

void str_to_lowercase_inplace(std::string& str) {
    for (size_t i = 0, len = str.size(); i < len; ++i) str[i] = std::tolower(str[i]);
}

std::string str_to_lowercase(std::string const& from) {
    std::string to;
    to.resize(from.size());
    for (size_t i = 0, len = from.size(); i < len; ++i) to[i] = std::tolower(from[i]);
    return to;
}

std::string str_to_lowercase(std::string_view const& from) {
    std::string to;
    to.resize(from.size());
    for (size_t i = 0, len = from.size(); i < len; ++i) to[i] = std::tolower(from[i]);
    return to;
}

bool str_ends_with(std::string const& str, std::string const& ends_with) {
    const auto sz = ends_with.size();
    return str.size() >= ends_with.size() && 0 == str.compare(str.size() - sz, sz, ends_with);
}

bool str_ends_with(std::string_view const& str, std::string const& ends_with) {
    const auto sz = ends_with.size();
    return str.size() >= ends_with.size() && 0 == str.compare(str.size() - sz, sz, ends_with);
}

void remove_trailing_zero(std::string& str) {
    auto trailing_zero_it = str.find_last_not_of('0');
    if (trailing_zero_it != std::string::npos) {
        if (str[trailing_zero_it] != '.') trailing_zero_it += 1;
        str.erase(trailing_zero_it, std::string::npos);
    }
}

uint64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string qty_to_str(qty_t q) {
    if (q >= 1000000000.0) {
        q = q / 1000000000.0;
        return fmt::format("{:.1f}B", q);
    }
    if (q >= 1000000.0) {
        q = q / 1000000.0;
        return fmt::format("{:.1f}M", q);
    }
    if (q >= 1000.0) {
        q = q / 1000.0;
        return fmt::format("{:.1f}K", q);
    }
    if (q < 1) {
        auto str = fmt::format("{:.5f}", q);
        remove_trailing_zero(str);
        return str;
    }
    return fmt::format("{:.1f}", q);
}

double precision_step_ceil(double step) {
    if (step <= 0.000001) return 0.000001;
    if (step <= 0.00001) return 0.00001;
    if (step <= 0.0001) return 0.0001;
    if (step <= 0.001) return 0.001;
    if (step <= 0.01) return 0.01;
    if (step <= 0.1) return 0.1;
    if (step <= 1) return 1;
    if (step <= 10) return 10;
    if (step <= 100) return 100;
    if (step <= 1000) return 1000;
    if (step <= 10000) return 10000;
    if (step <= 100000) return 100000;
    if (step <= 1000000) return 1000000;
    return step;
}

double precision_step_floor(double step) {
    if (step < 0.000001) return 0.0000001;
    if (step < 0.00001) return 0.000001;
    if (step < 0.0001) return 0.00001;
    if (step < 0.001) return 0.0001;
    if (step < 0.01) return 0.001;
    if (step < 0.1) return 0.01;
    if (step < 1) return 0.1;
    if (step < 10) return 1;
    if (step < 100) return 10;
    if (step < 1000) return 100;
    if (step < 10000) return 1000;
    if (step < 100000) return 10000;
    if (step < 1000000) return 100000;
    if (step < 10000000) return 1000000;
    return step;
}

bool str_contains_insensetive(std::string const& str, std::string const& substr) {
  auto it = std::search(
    str.begin(), str.end(),
    substr.begin(),   substr.end(),
    [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
  );
  return (it != str.end() );
}