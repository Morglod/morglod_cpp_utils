// rewrited version of https://github.com/gre/bezier-easing

#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <array>

#if _MSC_VER && !__INTEL_COMPILER
    #define NO_INLINE __declspec(noinline)
#else
    #define NO_INLINE __attribute__ ((noinline))
#endif

namespace {

template<typename val_t>
class Details {
public:
    static constexpr val_t NEWTON_ITERATIONS = 4;
    static constexpr val_t NEWTON_MIN_SLOPE = 0.001;
    static constexpr val_t SUBDIVISION_PRECISION = 0.0000001;
    static constexpr val_t SUBDIVISION_MAX_ITERATIONS = 10;

    static constexpr size_t SPLINE_TABLE_SIZE = 11;
    static constexpr val_t SAMPLE_STEP_SIZE = 1.0 / ((val_t)SPLINE_TABLE_SIZE - 1.0);

    inline static constexpr val_t A (val_t aA1, val_t aA2) { return 1.0 - 3.0 * aA2 + 3.0 * aA1; }
    inline static constexpr val_t B (val_t aA1, val_t aA2) { return 3.0 * aA2 - 6.0 * aA1; }
    inline static constexpr val_t C (val_t aA1)      { return 3.0 * aA1; }

    // Returns x(t) given t, x1, and x2, or y(t) given t, y1, and y2.
    NO_INLINE static constexpr val_t calcBezier (val_t aT, val_t aA1, val_t aA2) { return ((A(aA1, aA2) * aT + B(aA1, aA2)) * aT + C(aA1)) * aT; }

    // Returns dx/dt given t, x1, and x2, or dy/dt given t, y1, and y2.
    NO_INLINE static constexpr val_t getSlope (val_t aT, val_t aA1, val_t aA2) { return 3.0 * A(aA1, aA2) * aT * aT + 2.0 * B(aA1, aA2) * aT + C(aA1); }

    NO_INLINE static constexpr val_t binarySubdivide (val_t aX, val_t aA, val_t aB, val_t mX1, val_t mX2) {
        val_t currentX, currentT;
        int i = 0;
        do {
            currentT = aA + (aB - aA) / 2.0;
            currentX = calcBezier(currentT, mX1, mX2) - aX;
            if (currentX > 0.0) {
            aB = currentT;
            } else {
            aA = currentT;
            }
        } while (std::abs(currentX) > SUBDIVISION_PRECISION && ++i < SUBDIVISION_MAX_ITERATIONS);
        return currentT;
    }

    NO_INLINE static constexpr val_t newtonRaphsonIterate (val_t aX, val_t aGuessT, val_t mX1, val_t mX2) {
        for (int i = 0; i < NEWTON_ITERATIONS; ++i) {
        val_t currentSlope = getSlope(aGuessT, mX1, mX2);
        if (currentSlope == 0.0) {
            return aGuessT;
        }
        val_t currentX = calcBezier(aGuessT, mX1, mX2) - aX;
        aGuessT -= currentX / currentSlope;
        }
        return aGuessT;
    }

    inline static constexpr val_t LinearEasing (val_t x) {
        return x;
    }

    NO_INLINE static constexpr std::array<val_t, SPLINE_TABLE_SIZE> calc_sample_values(val_t mX1, val_t mX2) {
        std::array<val_t, SPLINE_TABLE_SIZE> sampleValues;
        for (int i = 0; i < SPLINE_TABLE_SIZE; ++i) {
            sampleValues[i] = calcBezier(i * SAMPLE_STEP_SIZE, mX1, mX2);
        }
        return sampleValues;
    }
};

}

template<typename val_t>
struct StaticBezierFuncParams {
    val_t mX1;
    val_t mY1;
    val_t mX2;
    val_t mY2;
};

template<typename val_t, const StaticBezierFuncParams<val_t>* const PARAMS>
class _StaticBezierFunc {
public:
    static constexpr val_t mX1 = PARAMS->mX1;
    static constexpr val_t mY1 = PARAMS->mY1;
    static constexpr val_t mX2 = PARAMS->mX2;
    static constexpr val_t mY2 = PARAMS->mY2;

    static constexpr std::array<val_t, Details<val_t>::SPLINE_TABLE_SIZE> sampleValues = Details<val_t>::calc_sample_values(mX1, mX2);

    inline static constexpr bool is_linear() {
        return mX1 == mY1 && mX2 == mY2;
    }

    NO_INLINE static constexpr val_t calc_curve(val_t aX) {
        val_t intervalStart = 0.0;
        int currentSample = 1;
        int lastSample = Details<val_t>::SPLINE_TABLE_SIZE - 1;

        for (; currentSample != lastSample && sampleValues[currentSample] <= aX; ++currentSample) {
            intervalStart += Details<val_t>::SAMPLE_STEP_SIZE;
        }
        --currentSample;

        // Interpolate to provide an initial guess for t
        val_t dist = (aX - sampleValues[currentSample]) / (sampleValues[currentSample + 1] - sampleValues[currentSample]);
        val_t guessForT = intervalStart + dist * Details<val_t>::SAMPLE_STEP_SIZE;

        val_t initialSlope = Details<val_t>::getSlope(guessForT, mX1, mX2);
        if (initialSlope >= Details<val_t>::NEWTON_MIN_SLOPE) {
            return Details<val_t>::newtonRaphsonIterate(aX, guessForT, mX1, mX2);
        } else if (initialSlope == 0.0) {
            return guessForT;
        } else {
            return Details<val_t>::binarySubdivide(aX, intervalStart, intervalStart + Details<val_t>::SAMPLE_STEP_SIZE, mX1, mX2);
        }
    }

    inline static constexpr val_t func(val_t x) {
        // skipped for performance
        // if (x <= 0) { return 0; }
        // if (x >= 1) { return 1; }

        if constexpr (is_linear()) return Details<val_t>::LinearEasing(x);
        return Details<val_t>::calcBezier(calc_curve(x), mY1, mY2);
    }

private:
    static constexpr void _compile_check_() {
        static_assert(!(0 <= mX1 && mX1 <= 1 && 0 <= mX2 && mX2 <= 1), "bezier x values must be in [0, 1] range");
    }
};

template<typename val_t>
class _DynamicBezierFunc {
public:
    const val_t mX1, mY1, mX2, mY2;

    const std::array<val_t, Details<val_t>::SPLINE_TABLE_SIZE> sampleValues;

    inline bool is_linear() const {
        return mX1 == mY1 && mX2 == mY2;
    }

    NO_INLINE val_t calc_curve(val_t aX) const {
        val_t intervalStart = 0.0;
        int currentSample = 1;
        int lastSample = Details<val_t>::SPLINE_TABLE_SIZE - 1;

        for (; currentSample != lastSample && sampleValues[currentSample] <= aX; ++currentSample) {
            intervalStart += Details<val_t>::SAMPLE_STEP_SIZE;
        }
        --currentSample;

        // Interpolate to provide an initial guess for t
        val_t dist = (aX - sampleValues[currentSample]) / (sampleValues[currentSample + 1] - sampleValues[currentSample]);
        val_t guessForT = intervalStart + dist * Details<val_t>::SAMPLE_STEP_SIZE;

        val_t initialSlope = Details<val_t>::getSlope(guessForT, mX1, mX2);
        if (initialSlope >= Details<val_t>::NEWTON_MIN_SLOPE) {
            return Details<val_t>::newtonRaphsonIterate(aX, guessForT, mX1, mX2);
        } else if (initialSlope == 0.0) {
            return guessForT;
        } else {
            return Details<val_t>::binarySubdivide(aX, intervalStart, intervalStart + Details<val_t>::SAMPLE_STEP_SIZE, mX1, mX2);
        }
    }

    inline val_t func(val_t x) const {
        // skipped for performance
        // if (x <= 0) { return 0; }
        // if (x >= 1) { return 1; }

        if constexpr (is_linear()) return Details<val_t>::LinearEasing(x);
        return Details<val_t>::calcBezier(calc_curve(x), mY1, mY2);
    }

    _DynamicBezierFunc(val_t mX1, val_t mY1, val_t mX2, val_t mY2) :
        mX1(mX1), mY1(mY1), mX2(mX2), mY2(mY2),
        sampleValues(Details<val_t>::calc_sample_values(mX1, mX2))
    {
        if (!(0 <= mX1 && mX1 <= 1 && 0 <= mX2 && mX2 <= 1)) throw std::runtime_error("bezier x values must be in [0, 1] range");
    }
};

constexpr float _f(float f) {
    return f;
}

template<const StaticBezierFuncParams<float>* const PARAMS>
using StaticBezierFuncF = _StaticBezierFunc<float, PARAMS>;

template<const StaticBezierFuncParams<double>* const PARAMS>
using StaticBezierFuncD = _StaticBezierFunc<double, PARAMS>;

using DynamicBezierFuncF = _DynamicBezierFunc<float>;
using DynamicBezierFuncD = _DynamicBezierFunc<double>;

constexpr StaticBezierFuncParams<float> _StaticBezierEaseF_PARAMS = { 0.25f, 0.1f, 0.25f, 1.0f };
constexpr StaticBezierFuncParams<float> _StaticBezierEaseInF_PARAMS = { 0.42f, 0.0f, 1.0f, 1.0f };
constexpr StaticBezierFuncParams<float> _StaticBezierEaseOutF_PARAMS = { 0.42f, 0.0f, 0.58f, 1.0f };
constexpr StaticBezierFuncParams<float> _StaticBezierEaseInOutF_PARAMS = { 0.0f, 0.0f, 0.58f, 1.0f };

constexpr StaticBezierFuncParams<double> _StaticBezierEaseD_PARAMS = { 0.25, 0.1, 0.25, 1.0 };
constexpr StaticBezierFuncParams<double> _StaticBezierEaseInD_PARAMS = { 0.42, 0.0, 1.0, 1.0 };
constexpr StaticBezierFuncParams<double> _StaticBezierEaseOutD_PARAMS = { 0.42, 0.0, 0.58, 1.0 };
constexpr StaticBezierFuncParams<double> _StaticBezierEaseInOutD_PARAMS = { 0.0, 0.0, 0.58, 1.0 };

using StaticBezierEaseF = _StaticBezierFunc<float, &_StaticBezierEaseF_PARAMS>;
using StaticBezierEaseInF = _StaticBezierFunc<float, &_StaticBezierEaseInF_PARAMS>;
using StaticBezierEaseOutF = _StaticBezierFunc<float, &_StaticBezierEaseOutF_PARAMS>;
using StaticBezierEaseInOutF = _StaticBezierFunc<float, &_StaticBezierEaseInOutF_PARAMS>;

using StaticBezierEaseD = _StaticBezierFunc<double, &_StaticBezierEaseD_PARAMS>;
using StaticBezierEaseInD = _StaticBezierFunc<double, &_StaticBezierEaseInD_PARAMS>;
using StaticBezierEaseOutD = _StaticBezierFunc<double, &_StaticBezierEaseOutD_PARAMS>;
using StaticBezierEaseInOutD = _StaticBezierFunc<double, &_StaticBezierEaseInOutD_PARAMS>;