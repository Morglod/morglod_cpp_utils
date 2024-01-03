#pragma once

#include "./_globals.hpp"

#include <string>

#ifndef _PROFILER_ENABLED
#define _PROFILER_ENABLED 0
#endif

#if _PROFILER_ENABLED == 1

#include <chrono>
#include "./log.hpp"
#include "./utils.hpp"

#define _PROFILER_START(_NAME, ...) \
    const auto __123__profiler_start_time_##__VA_ARGS__ = std::chrono::high_resolution_clock::now(); \
    profiler_start(_NAME);

#define _PROFILER_END(_NAME, ...) \
    const auto __123__profiler_end_time_##__VA_ARGS__ = std::chrono::high_resolution_clock::now(); \
    profiler_end(_NAME, (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(__123__profiler_end_time_##__VA_ARGS__ - __123__profiler_start_time_##__VA_ARGS__).count());

#define _PROFILER_AUTO(...) \
    const auto __123__profiler_start_time_##__VA_ARGS__ = std::chrono::high_resolution_clock::now(); \
    profiler_start(std::string(_KS_LOG_CLEAN_FUNC_NAME) + #__VA_ARGS__); \
    _DEFER_DO([__123__profiler_start_time_##__VA_ARGS__]() { \
        const auto __123__profiler_end_time_##__VA_ARGS__ = std::chrono::high_resolution_clock::now(); \
        profiler_end(std::string(_KS_LOG_CLEAN_FUNC_NAME) + #__VA_ARGS__, (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(__123__profiler_end_time_##__VA_ARGS__ - __123__profiler_start_time_##__VA_ARGS__).count()); \
    });

#else

#define _PROFILER_START(_NAME, ...)
#define _PROFILER_END(_NAME, ...)
#define _PROFILER_AUTO(...)

#endif

void profiler_start(std::string const& name);
void profiler_end(std::string const& name, uint64_t const& time_microsec);
void profiler_print();

#define _MEASURE_TIME_START(_NAME, _I) \
    const auto __123__measure_time_start_time_##_I = std::chrono::high_resolution_clock::now();

#define _MEASURE_TIME_END(_NAME, _I) \
    const auto __123__measure_time_end_time_##_I = std::chrono::high_resolution_clock::now(); \
    KS_LOG(_NAME, "micros=", (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(__123__measure_time_end_time_##_I - __123__measure_time_start_time_##_I).count());