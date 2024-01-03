// #pragma once

// #include <cstddef>
// #include <emmintrin.h>
// #include <bit>
// #include <limits>

// namespace bits {
//     template <typename T>
//     inline T clear_leftmost_set(const T value) {
//         return value & (value - 1);
//     }

//     template <typename T>
//     inline unsigned get_first_bit_set(const T value) {
//         return __builtin_ctz(value);
//     }

//     template <>
//     inline unsigned get_first_bit_set<uint64_t>(const uint64_t value) {
//         return __builtin_ctzl(value);
//     }
// } // namespace bits

// constexpr const char _json_scan_string_buf[17] = { "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"" };
// const __m128i _json_util_skip_string_m128i = _mm_loadu_si128((const __m128i *)_json_scan_string_buf);

// constexpr const char _json_scan_brace_open_buf[17] = { "{{{{{{{{{{{{{{{{" };
// const __m128i _json_scan_brace_open_buf_m128i = _mm_loadu_si128((const __m128i *)_json_scan_brace_open_buf);

// constexpr const char _json_scan_brace_close_buf[17] = { "}}}}}}}}}}}}}}}}" };
// const __m128i _json_scan_brace_close_buf_m128i = _mm_loadu_si128((const __m128i *)_json_scan_brace_close_buf);

// const int unsafe_json_calc_obj_depth_no_leave_npos = std::numeric_limits<int>::min();

// inline const char* _json_util_find_part_with_depth(
//     const char* s, const char* limit,
//     const char* part_text, int part_text_size,
//     int depth
// ) {
//     int current_depth = 0;

//     for (; s < limit - 16; s += 16) {
//         const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));

//         const __m128i string_skips = _mm_cmpeq_epi8(a, _json_util_skip_string_m128i);
//         uint16_t string_skips_mask = _mm_movemask_epi8(string_skips);

//         const __m128i opens_in_a = _mm_cmpeq_epi8(a, _json_scan_brace_open_buf_m128i);
//         uint16_t opens_in_a_mask = _mm_movemask_epi8(opens_in_a);

//         depth += std::popcount(opens_in_a_mask);

//         const __m128i closes_in_a = _mm_cmpeq_epi8(a, _json_scan_brace_close_buf_m128i);
//         uint16_t closes_in_a_mask = _mm_movemask_epi8(closes_in_a);

//         depth -= std::popcount(opens_in_a_mask);

//         if (depth < 0) {
//             // we left current object
//             return unsafe_json_calc_obj_depth_no_leave_npos;
//         }
//     }

//     return depth;
// }
