#pragma once

#include <memory>

#define FORCE_INLINE inline __attribute__((always_inline))
#define MAYBE_UNUSED inline __attribute__((unused))

#define _MEMCPY_RESTRICT __restrict__

namespace {
    MAYBE_UNUSED
    bool always_true(const char*, const char*) {
        return true;
    }

    MAYBE_UNUSED
    bool memcmp1(const char* a, const char* b) {
        return a[0] == b[0];
    }

    MAYBE_UNUSED
    bool memcmp2(const char* a, const char* b) {
        const uint16_t A = *reinterpret_cast<const uint16_t*>(a);
        const uint16_t B = *reinterpret_cast<const uint16_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp3(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp2(a, b) && memcmp1(a + 2, b + 2);
#else
        const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        return (A & 0x00ffffff) == (B & 0x00ffffff);
#endif
    }

    MAYBE_UNUSED
    bool memcmp4(const char* a, const char* b) {

        const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp5(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp4(a, b) && memcmp1(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x000000fffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp6(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp4(a, b) && memcmp2(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x0000fffffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp7(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP 
        return memcmp4(a, b) && memcmp3(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x00fffffffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp8(const char* a, const char* b) {

        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp9(const char* a, const char* b) {

        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return (A == B) & (a[8] == b[8]);
    }

    MAYBE_UNUSED
    bool memcmp10(const char* a, const char* b) {

        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint16_t Aw = *reinterpret_cast<const uint16_t*>(a + 8);
        const uint16_t Bw = *reinterpret_cast<const uint16_t*>(b + 8);
        return (Aq == Bq) & (Aw == Bw);
    }

    MAYBE_UNUSED
    bool memcmp11(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp8(a, b) && memcmp3(a + 8, b + 8);
#else
        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
        const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
        return (Aq == Bq) & ((Ad & 0x00ffffff) == (Bd & 0x00ffffff));
#endif
    }

    MAYBE_UNUSED
    bool memcmp12(const char* a, const char* b) {

        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
        const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
        return (Aq == Bq) & (Ad == Bd);
    }
}

template<size_t B_SIZE>
inline bool precompile_memcmp(const char* a, const char* b) {
    if constexpr (B_SIZE == 0) return true;
    if constexpr (B_SIZE == 1) return memcmp1(a, b);
    if constexpr (B_SIZE == 2) return memcmp2(a, b);
    if constexpr (B_SIZE == 3) return memcmp3(a, b);
    if constexpr (B_SIZE == 4) return memcmp4(a, b);
    if constexpr (B_SIZE == 5) return memcmp5(a, b);
    if constexpr (B_SIZE == 6) return memcmp6(a, b);
    if constexpr (B_SIZE == 7) return memcmp7(a, b);
    if constexpr (B_SIZE == 8) return memcmp8(a, b);
    if constexpr (B_SIZE == 9) return memcmp9(a, b);
    if constexpr (B_SIZE == 10) return memcmp10(a, b);
    if constexpr (B_SIZE == 11) return memcmp11(a, b);
    if constexpr (B_SIZE == 12) return memcmp12(a, b);
    return memcmp(a, b, B_SIZE) == 0;
}

inline bool switch_memcmp(const char* a, const char* b, size_t b_size) {
    switch (b_size) {
        case 0: return true;
        case 1: return memcmp1(a, b);
        case 2: return memcmp2(a, b);
        case 3: return memcmp3(a, b);
        case 4: return memcmp4(a, b);
        case 5: return memcmp5(a, b);
        case 6: return memcmp6(a, b);
        case 7: return memcmp7(a, b);
        case 8: return memcmp8(a, b);
        case 9: return memcmp9(a, b);
        case 10: return memcmp10(a, b);
        case 11: return memcmp11(a, b);
        case 12: return memcmp12(a, b);
    }
    return memcmp(a, b, b_size) == 0;
}

// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------
// -----------------------------------

namespace {
    MAYBE_UNUSED
    void memcpy1(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        a[0] = b[0];
    }

    MAYBE_UNUSED
    void memcpy2(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint16_t* A = reinterpret_cast<uint16_t*>(a);
        const uint16_t B = *reinterpret_cast<const uint16_t*>(b);
        *A = B;
    }

    MAYBE_UNUSED
    void memcpy3(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2];
    }

    MAYBE_UNUSED
    void memcpy4(char* a, const char* b) {
        uint32_t* A = reinterpret_cast<uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        *A = B;
    }

    MAYBE_UNUSED
    void memcpy5(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint32_t* A = reinterpret_cast<uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        *A = B;
        a[4] = b[4];
    }

    MAYBE_UNUSED
    void memcpy6(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint16_t* A = reinterpret_cast<uint16_t*>(a);
        const uint16_t* B = reinterpret_cast<const uint16_t*>(b);
        A[0] = B[0];
        A[1] = B[1];
        A[2] = B[2];
    }

    MAYBE_UNUSED
    void memcpy7(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint16_t* A = reinterpret_cast<uint16_t*>(a);
        const uint16_t* B = reinterpret_cast<const uint16_t*>(b);
        A[0] = B[0];
        A[1] = B[1];
        A[2] = B[2];
        a[6] = b[6];
    }

    MAYBE_UNUSED
    void memcpy8(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint64_t* A = reinterpret_cast<uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        *A = B;
    }

    MAYBE_UNUSED
    void memcpy9(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint64_t* A = reinterpret_cast<uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        *A = B;
        a[8] = b[8];
    }

    MAYBE_UNUSED
    void memcpy10(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint64_t* Aq = reinterpret_cast<uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        uint16_t* Aw = reinterpret_cast<uint16_t*>(a + 8);
        const uint16_t Bw = *reinterpret_cast<const uint16_t*>(b + 8);
        *Aq = Bq;
        *Aw = Bw;
    }

    MAYBE_UNUSED
    void memcpy11(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint64_t* Aq = reinterpret_cast<uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        uint16_t* Aw = reinterpret_cast<uint16_t*>(a + 8);
        const uint16_t Bw = *reinterpret_cast<const uint16_t*>(b + 8);
        *Aq = Bq;
        *Aw = Bw;
        a[10] = b[10];
    }

    MAYBE_UNUSED
    void memcpy12(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
        uint64_t* Aq = reinterpret_cast<uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        uint32_t* Ad = reinterpret_cast<uint32_t*>(a + 8);
        const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
        *Aq = Bq;
        *Ad = Bd;
    }
}

template<size_t B_SIZE>
inline void precompile_memcpy(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b) {
    if constexpr (B_SIZE == 1) { memcpy1(a, b); return; }
    if constexpr (B_SIZE == 2) { memcpy2(a, b); return; }
    if constexpr (B_SIZE == 3) { memcpy3(a, b); return; }
    if constexpr (B_SIZE == 4) { memcpy4(a, b); return; }
    if constexpr (B_SIZE == 5) { memcpy5(a, b); return; }
    if constexpr (B_SIZE == 6) { memcpy6(a, b); return; }
    if constexpr (B_SIZE == 7) { memcpy7(a, b); return; }
    if constexpr (B_SIZE == 8) { memcpy8(a, b); return; }
    if constexpr (B_SIZE == 9) { memcpy9(a, b); return; }
    if constexpr (B_SIZE == 10) { memcpy10(a, b); return; }
    if constexpr (B_SIZE == 11) { memcpy11(a, b); return; }
    if constexpr (B_SIZE == 12) { memcpy12(a, b); return; }
    memcpy(a, b, B_SIZE);
}

inline void switch_memcpy(char* _MEMCPY_RESTRICT a, const char* _MEMCPY_RESTRICT b, size_t b_size) {
    switch (b_size) {
        case 0: return;
        case 1: { memcpy1(a, b); return; }
        case 2: { memcpy2(a, b); return; }
        case 3: { memcpy3(a, b); return; }
        case 4: { memcpy4(a, b); return; }
        case 5: { memcpy5(a, b); return; }
        case 6: { memcpy6(a, b); return; }
        case 7: { memcpy7(a, b); return; }
        case 8: { memcpy8(a, b); return; }
        case 9: { memcpy9(a, b); return; }
        case 10: { memcpy10(a, b); return; }
        case 11: { memcpy11(a, b); return; }
        case 12: { memcpy12(a, b); return; }
    }
    memcpy(a, b, b_size);
}

template<size_t B_SIZE>
inline void precompile_memcpy_str(char* _MEMCPY_RESTRICT a, const char (&b)[B_SIZE]) {
    constexpr size_t str_len = B_SIZE % 2 == 0 ? B_SIZE : B_SIZE - 1; // optionally skip zero char
    if constexpr (str_len == 1) { memcpy1(a, b); return; }
    if constexpr (str_len == 2) { memcpy2(a, b); return; }
    if constexpr (str_len == 3) { memcpy3(a, b); return; }
    if constexpr (str_len == 4) { memcpy4(a, b); return; }
    if constexpr (str_len == 5) { memcpy5(a, b); return; }
    if constexpr (str_len == 6) { memcpy6(a, b); return; }
    if constexpr (str_len == 7) { memcpy7(a, b); return; }
    if constexpr (str_len == 8) { memcpy8(a, b); return; }
    if constexpr (str_len == 9) { memcpy9(a, b); return; }
    if constexpr (str_len == 10) { memcpy10(a, b); return; }
    if constexpr (str_len == 11) { memcpy11(a, b); return; }
    if constexpr (str_len == 12) { memcpy12(a, b); return; }
    memcpy(a, b, str_len);
}
