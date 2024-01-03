#pragma once

#if _MSC_VER && !__INTEL_COMPILER
    #define NO_INLINE __declspec(noinline)
#else
    #define NO_INLINE __attribute__ ((noinline))
#endif