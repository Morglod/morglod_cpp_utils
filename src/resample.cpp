#include "./resample.hpp"
#include <cstring>
#include <stdexcept>

// we assume that ValueT is trivial copyable

template<typename ValueT>
void resample_t(
    const ValueT* from_begin, size_t const& from_size,
    ValueT* to_begin, size_t const& to_size
) {
    if (from_size == to_size) {
        std::memcpy(to_begin, from_begin, from_size);
        return;
    }

    // TODO: rewrite this shit fully

    throw std::runtime_error("rewrite this shit fully");

    // shrink case
    if (from_size > to_size) {
        // part of real item per sampled
        ValueT sampling_rate = (ValueT)to_size / (ValueT)from_size;

        size_t from_i = 0;
        ValueT* to_end = to_begin + to_size;
        for (ValueT* to_it = to_begin; to_it != to_end; ++to_it) {
            *to_it = (*from_begin + from_i) * sampling_rate;
        }
    }
}

void resample(
    const float* from_begin, size_t const& from_size,
    float* to_begin, size_t const& to_size
) {
    return resample_t<float>(from_begin, from_size, to_begin, to_size);
}

void resample(
    const double* from_begin, size_t const& from_size,
    double* to_begin, size_t const& to_size
) {
    return resample_t<double>(from_begin, from_size, to_begin, to_size);
}