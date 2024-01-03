#pragma once

#include <cstddef>

void resample(
    const float* from_begin, size_t const& from_size,
    float* to_begin, size_t const& to_size
);

void resample(
    const double* from_begin, size_t const& from_size,
    double* to_begin, size_t const& to_size
);