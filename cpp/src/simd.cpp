#include "sentinel/simd.hpp"

#include <cassert>
#include <cstddef>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace sentinel::simd {

float dot_scalar(std::span<const float> a, std::span<const float> b) noexcept {
    assert(a.size() == b.size());
    float acc = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i) {
        acc += a[i] * b[i];
    }
    return acc;
}

float dot(std::span<const float> a, std::span<const float> b) noexcept {
    assert(a.size() == b.size());
#if defined(__ARM_NEON)
    const std::size_t n = a.size();
    const float* pa = a.data();
    const float* pb = b.data();

    // Four independent accumulators (16 floats per iteration) hide FMA latency:
    // a single accumulator would serialise every fused multiply-add on the
    // previous one. This is the kind of choice we will measure, not assume.
    float32x4_t acc0 = vdupq_n_f32(0.0f);
    float32x4_t acc1 = vdupq_n_f32(0.0f);
    float32x4_t acc2 = vdupq_n_f32(0.0f);
    float32x4_t acc3 = vdupq_n_f32(0.0f);

    std::size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        acc0 = vfmaq_f32(acc0, vld1q_f32(pa + i), vld1q_f32(pb + i));
        acc1 = vfmaq_f32(acc1, vld1q_f32(pa + i + 4), vld1q_f32(pb + i + 4));
        acc2 = vfmaq_f32(acc2, vld1q_f32(pa + i + 8), vld1q_f32(pb + i + 8));
        acc3 = vfmaq_f32(acc3, vld1q_f32(pa + i + 12), vld1q_f32(pb + i + 12));
    }
    for (; i + 4 <= n; i += 4) {
        acc0 = vfmaq_f32(acc0, vld1q_f32(pa + i), vld1q_f32(pb + i));
    }

    float total = vaddvq_f32(vaddq_f32(vaddq_f32(acc0, acc1), vaddq_f32(acc2, acc3)));
    for (; i < n; ++i) {  // scalar tail
        total += pa[i] * pb[i];
    }
    return total;
#else
    return dot_scalar(a, b);
#endif
}

}  // namespace sentinel::simd
