#pragma once

#include <stdexcept>
#include <stdint.h>
#include <string>
#include <cmath>
#include <variant>

#include "./types.hpp"

#define _TEXT_SIZE( TEXT ) (sizeof( TEXT ) - 1)
#define _TEXT_AND_SIZE( TEXT ) TEXT, _TEXT_SIZE( TEXT )

inline constexpr void _static_assert_TEXT_SIZE() {
    static_assert(_TEXT_SIZE("hello") == 5, "_TEXT_SIZE should return num of chars excluding zero char");
}

void str_to_uppercase_inplace(std::string& str);
std::string str_to_uppercase(std::string const& from);
std::string str_to_uppercase(std::string_view const& from);
void str_to_lowercase_inplace(std::string& str);
std::string str_to_lowercase(std::string const& from);
std::string str_to_lowercase(std::string_view const& from);
bool str_contains_insensetive(std::string const& str, std::string const& substr);

bool str_ends_with(std::string const& str, std::string const& ends_with);
bool str_ends_with(std::string_view const& str, std::string const& ends_with);
void remove_trailing_zero(std::string& str);

std::string qty_to_str(qty_t q);

double precision_step_ceil(double step);
double precision_step_floor(double step);

inline int8_t precision_of(double const& prec) {
    return (int8_t)std::log10(std::ceil(1.0 / prec));
}

inline double floor_by_precision(double const& v, int8_t const& prec) {
    double_t mul = std::pow(10.0, (double)prec);
    uint64_t floored = (uint64_t)(v * mul);
    return (double)floored / mul; 
}

// precision_floor(10.126, 0.01) -> 10.12
inline double precision_floor(double const& v, double const& prec) {
    return std::floor(v / prec) * prec;
}

// precision_floor(10.126, 0.01) -> 10.13
inline double precision_round(double const& v, double const& prec) {
    return std::round(v / prec) * prec;
}

// precision_floor(10.126, 0.01) -> 10.13
inline double precision_ceil(double const& v, double const& prec) {
    return std::ceil(v / prec) * prec;
}

uint64_t get_current_time_ms();

// Example:
// step over all volumes by ptr math
// _FOR_EACH_PTR_STEP(&buffer[0].volume, price_levels_num, sizeof(PriceLevel), qty_t,
//     *it = 0;
// )
#define _FOR_EACH_PTR_STEP(_PTR_FIRST_ITEM, _NUM_STEPS, _STEP_SIZE, _ITEM_TYPE, _DO) \
    for (char* _it = (char*)(void*)( _PTR_FIRST_ITEM ), *_it_end = (char*)(void*)( _PTR_FIRST_ITEM ) + (_STEP_SIZE * _NUM_STEPS); _it != _it_end; _it += _STEP_SIZE) { \
        _ITEM_TYPE* it = (_ITEM_TYPE *)(void*)_it; \
        _DO \
    }


// Use for(int ... or auto it = begin) with vectors
// This gives performance only with buffers
#define _FOR_EACH_PTR_NUM(_PTR_FIRST_ITEM, _TOTAL_ITEMS_NUM) \
    for (auto it = _PTR_FIRST_ITEM, it_end = _PTR_FIRST_ITEM + _TOTAL_ITEMS_NUM; it != it_end; ++it)

#define SHOULD_FIT_CACHE_LINE(_TYPE) \
    struct _ShouldFitCacheLine_##_TYPE { static_assert(sizeof(_TYPE) <= 64, #_TYPE " does not fit cache line (64 bytes)"); };

// this value is the only value in cache line
template<typename T>
struct alignas(64) CacheLineVal {
    T value;
};

#define CACHE_LINE_PACK(_NAME, _FIELDS) \
    struct alignas(64) _NAME { \
        using Self = _NAME; \
        _FIELDS \
    }; \
    SHOULD_FIT_CACHE_LINE(_NAME)

template<typename Func>
auto _CREATE_DEFER(Func func) {
    struct _Defer {
        Func f;
        inline ~_Defer() { f(); }
    };
    return _Defer{func};
}

// ensure that this fields will be in same cache line
#define CACHE_LOCALITY(_FEILDS) \
    alignas(64) _FIELDS \
    CACHE_LINE_PACK(CACHE_LOCALITY##_LINE_, _FIELDS)

// run code on scope exit
// eg:
// _DEFER_DO([]() {
// 	profiler_print();
// });
#define _DEFER_DO(_FUNC) \
    auto _defer_do_##_LINE_ = _CREATE_DEFER(_FUNC);

// run code on scope exit
// eg:
// _DEFER( profiler_print(); );
#define _DEFER(_EXPR) \
    auto _defer_do_##_LINE_ = _CREATE_DEFER([=](){ _EXPR });

template<typename std_variant_t, typename T>
struct variant_type_index {
    static constexpr size_t get_index() noexcept {
        #if _MSC_VER && !__INTEL_COMPILER
            return std::_Meta_find_unique_index<std_variant_t, T>::value;
        #else
            // TODO: support other compilers
            throw std::runtime_error("variant_type_index::get_index not supported");
            return 0;
        #endif
    }
};

#if _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#else
    // #define __PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#endif

template<typename T>
struct ManualRAII {
    union { T value; };

    template<typename... ArgsT>
    inline void constructor(ArgsT ...args) { new (&value) T (args...); }
    inline void destructor() { value.~T(); }

    inline ManualRAII() noexcept {}
    inline ~ManualRAII() noexcept {}
};

template<typename T>
inline void delete_at(T*& at) {
    delete at;
    at = nullptr;
}

template<typename TargetT>
inline TargetT safe_conv_size(size_t sz) {
    if (sz > std::numeric_limits<TargetT>::max()) {
        throw "value of size_t is larger that TargetT";
    }
    return (TargetT)sz;
}

template<typename T>
inline void append_vector(std::vector<T>& append_to, std::vector<T> const& append_from) {
    append_to.insert( append_to.end(), append_from.begin(), append_from.end() );
}

template<typename T, typename T2>
inline void append_vector(std::vector<std::shared_ptr<T>>& append_to, std::vector<std::shared_ptr<T2>> const& append_from) {
    for (auto const& it : append_from) {
        std::shared_ptr<T> conv = std::static_pointer_cast<T, T2>(it);
        append_to.push_back(conv);
    }
}
