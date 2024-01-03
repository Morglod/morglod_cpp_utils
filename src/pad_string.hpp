// #pragma once

// #include "./crc32.hpp"
// #include "./profiler.hpp"

// #include <arm_neon.h>

// #include <cstring>
// #include <string>
// #include <memory>
// #include <emmintrin.h>

// namespace {

// constexpr size_t get_aligned_padding_size() {
//     #ifdef __SSE2__
//         return 16; // 128 bit
//     #endif
//     return sizeof(void*);
// }

// constexpr size_t get_aligned_padding_length(size_t size) {
//     size_t p = get_aligned_padding_size();
//     return size + size % p;
// }

// }

// typedef const char* aligned_const_cstr;

// struct aligned_string_buf {
//     char* buf = nullptr;
//     char* buf_end = nullptr;

//     inline aligned_const_cstr c_str() const {
//         return buf;
//     }

//     inline auto size() const {
//         return buf_end - buf;
//     }

//     inline constexpr aligned_string_buf() = default;

//     aligned_string_buf(aligned_string_buf const&) = delete;

//     inline aligned_string_buf(size_t size) {
//         size_t align = get_aligned_padding_size();
//         size_t padded_size = get_aligned_padding_length(size);
//         buf = (char*)aligned_alloc(align, padded_size);
//         buf_end = buf + padded_size;
//     }

//     inline aligned_string_buf(const char* str, size_t str_size, size_t reserved_size) {
//         size_t align = get_aligned_padding_size();
//         size_t padded_size = get_aligned_padding_length(reserved_size);
//         buf = (char*)aligned_alloc(align, padded_size);
//         buf_end = buf + padded_size;

//         memcpy(buf, str, str_size);
//     }

//     inline aligned_string_buf&& copy() {
//         return std::move(aligned_string_buf(buf, size(), buf_end - buf));
//     }

//     inline ~aligned_string_buf() {
//         free(buf);
//         buf = nullptr;
//         buf_end = nullptr;
//     }
// };

// typedef std::shared_ptr<aligned_string_buf> aligned_string_buf_ptr;

// #define ALIGNED_STATIC_STR( STR ) \
//     std::string( STR ) + "\0" + std::string(get_aligned_padding_size() % strlen(STR) );

// template<size_t N>
// inline bool aligned_memcmp_static(aligned_const_cstr __restrict__ a, const char (&b)[N]) {
//     static_assert(N % get_aligned_padding_size() != 0, "b is not properly aligned!");

//     _PROFILER_START("aligned_memcmp");
//     #ifdef __SSE2__
//         __m128i xmm0, xmm1, xmm2;
//         unsigned int eax;

//         for (; b != b_end; a += get_aligned_padding_size(), b += get_aligned_padding_size()) {
//             xmm0 = _mm_loadu_si128((const __m128i *)a);
//             xmm1 = _mm_loadu_si128((const __m128i *)b);
//             xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);
//             eax = _mm_movemask_epi8(xmm2);
//             if (eax != UINT_MAX) {
//                 _JSON_WALK_PROFILER_END("aligned_memcmp");
//                 return false;
//             }
//         }
//     #else
//         if (memcmp(a, b, N) != 0) {
//             _PROFILER_END("aligned_memcmp");
//             return false;
//         }
//     #endif

//     _PROFILER_END("aligned_memcmp");
//     return true;
// }

// inline bool aligned_memcmp_dyn(aligned_const_cstr __restrict__ a, aligned_const_cstr __restrict__ b, aligned_const_cstr b_end) {
//     _PROFILER_START("aligned_memcmp");
//     #ifdef __SSE2__
//         __m128i xmm0, xmm1, xmm2;
//         unsigned int eax;

//         for (; b != b_end; a += get_aligned_padding_size(), b += get_aligned_padding_size()) {
//             xmm0 = _mm_loadu_si128((const __m128i *)a);
//             xmm1 = _mm_loadu_si128((const __m128i *)b);
//             xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);
//             eax = _mm_movemask_epi8(xmm2);
//             if (eax != UINT_MAX) {
//                 _JSON_WALK_PROFILER_END("aligned_memcmp");
//                 return false;
//             }
//         }
//     #else
//         if (memcmp(a, b, b_end - b) != 0) {
//             _PROFILER_END("aligned_memcmp");
//             return false;
//         }
//     #endif

//     _PROFILER_END("aligned_memcmp");
//     return true;
// }

// inline aligned_const_cstr aligned_find_mem(aligned_const_cstr __restrict__ a, aligned_const_cstr __restrict__ a_end, aligned_const_cstr __restrict__ b, aligned_const_cstr b_end) {
//     _PROFILER_START("aligned_memcmp");
//     #ifdef __SSE2__
//         __m128i xmm0, xmm1, xmm2;
//         unsigned int eax;

//         for (; b != b_end; a += get_aligned_padding_size(), b += get_aligned_padding_size()) {
//             xmm0 = _mm_loadu_si128((const __m128i *)a);
//             xmm1 = _mm_loadu_si128((const __m128i *)b);
//             xmm2 = _mm_cmpeq_epi8(xmm0, xmm1);
//             eax = _mm_movemask_epi8(xmm2);
//             if (eax != UINT_MAX) {
//                 _JSON_WALK_PROFILER_END("aligned_memcmp");
//                 return false;
//             }
//         }
//     #else
//         char b_last = *(b_end - 1);
//         auto b_len = b_end - b;
//         for (; a != a_end; ++a) {
//             if (*a == *b && *(a + b_len) == b_last) {
//                 if (aligned_memcmp_dyn(a, b, b_end)) {
//                     return a;
//                 }
//             }
//         }
//     #endif

//     _PROFILER_END("aligned_memcmp");
//     return nullptr;
// }

// // ----------------------

// namespace bits {

//     template <typename T>
//     T clear_leftmost_set(const T value) {
//         assert(value != 0);
//         return value & (value - 1);
//     }


//     template <typename T>
//     unsigned get_first_bit_set(const T value) {
//         assert(value != 0);
//         return __builtin_ctz(value);
//     }


//     template <>
//     unsigned get_first_bit_set<uint64_t>(const uint64_t value) {
//         assert(value != 0);
//         return __builtin_ctzl(value);
//     }

// } // namespace bits

// size_t inline sse2_strstr_anysize(const char* s, size_t n, const char* needle, size_t k) {

//     assert(k > 0);
//     assert(n > 0);

//     const __m128i first = _mm_set1_epi8(needle[0]);
//     const __m128i last  = _mm_set1_epi8(needle[k - 1]);

//     for (size_t i = 0; i < n; i += 16) {

//         const __m128i block_first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i));
//         const __m128i block_last  = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i + k - 1));

//         const __m128i eq_first = _mm_cmpeq_epi8(first, block_first);
//         const __m128i eq_last  = _mm_cmpeq_epi8(last, block_last);

//         uint16_t mask = _mm_movemask_epi8(_mm_and_si128(eq_first, eq_last));

//         while (mask != 0) {

//             const auto bitpos = bits::get_first_bit_set(mask);

//             if (memcmp(s + i + bitpos + 1, needle + 1, k - 2) == 0) {
//                 return i + bitpos;
//             }

//             mask = bits::clear_leftmost_set(mask);
//         }
//     }

//     return std::string::npos;
// }

// // ------------------------------------------------------------------------

// template <size_t k, typename MEMCMP>
// size_t inline sse2_strstr_memcmp(const char* s, size_t n, const char* needle, MEMCMP memcmp_fun) {

//     assert(k > 0);
//     assert(n > 0);

//     const __m128i first = _mm_set1_epi8(needle[0]);
//     const __m128i last  = _mm_set1_epi8(needle[k - 1]);

//     for (size_t i = 0; i < n; i += 16) {

//         const __m128i block_first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i));
//         const __m128i block_last  = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i + k - 1));

//         const __m128i eq_first = _mm_cmpeq_epi8(first, block_first);
//         const __m128i eq_last  = _mm_cmpeq_epi8(last, block_last);

//         uint32_t mask = _mm_movemask_epi8(_mm_and_si128(eq_first, eq_last));

//         while (mask != 0) {

//             const auto bitpos = bits::get_first_bit_set(mask);

//             if (memcmp_fun(s + i + bitpos + 1, needle + 1)) {
//                 return i + bitpos;
//             }

//             mask = bits::clear_leftmost_set(mask);
//         }
//     }

//     return std::string::npos;
// }

// // ------------------------------------------------------------------------

// size_t sse2_strstr_v2(const char* s, size_t n, const char* needle, size_t k) {

//     size_t result = std::string::npos;

//     if (n < k) {
//         return result;
//     }

// 	switch (k) {
// 		case 0:
// 			return 0;

// 		case 1: {
//             const char* res = reinterpret_cast<const char*>(strchr(s, needle[0]));

// 			return (res != nullptr) ? res - s : std::string::npos;
//             }

//         case 2:
//             result = sse2_strstr_memcmp<2>(s, n, needle, always_true);
//             break;

//         case 3:
//             result = sse2_strstr_memcmp<3>(s, n, needle, memcmp1);
//             break;

//         case 4:
//             result = sse2_strstr_memcmp<4>(s, n, needle, memcmp2);
//             break;

//         case 5:
//             result = sse2_strstr_memcmp<5>(s, n, needle, memcmp4);
//             break;

//         case 6:
//             result = sse2_strstr_memcmp<6>(s, n, needle, memcmp4);
//             break;

//         case 7:
//             result = sse2_strstr_memcmp<7>(s, n, needle, memcmp5);
//             break;

//         case 8:
//             result = sse2_strstr_memcmp<8>(s, n, needle, memcmp6);
//             break;

//         case 9:
//             result = sse2_strstr_memcmp<9>(s, n, needle, memcmp8);
//             break;

//         case 10:
//             result = sse2_strstr_memcmp<10>(s, n, needle, memcmp8);
//             break;

//         case 11:
//             result = sse2_strstr_memcmp<11>(s, n, needle, memcmp9);
//             break;

//         case 12:
//             result = sse2_strstr_memcmp<12>(s, n, needle, memcmp10);
//             break;

// 		default:
// 			result = sse2_strstr_anysize(s, n, needle, k);
//             break;
//     }

//     if (result <= n - k) {
//         return result;
//     } else {
//         return std::string::npos;
//     }
// }