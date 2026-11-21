#pragma once

#include <span>

namespace sentinel::simd {

// True when this translation unit was compiled with NEON available.
// On every Apple Silicon Mac this should be true.
[[nodiscard]] constexpr bool neon_enabled() noexcept {
#if defined(__ARM_NEON)
    return true;
#else
    return false;
#endif
}

// Reference implementation: plain sequential loop.
// Precondition: a.size() == b.size().
[[nodiscard]] float dot_scalar(std::span<const float> a, std::span<const float> b) noexcept;

// NEON implementation when available, otherwise falls back to dot_scalar.
// Results may differ from dot_scalar in the last bits because floating-point
// addition is not associative and this version reorders the summation.
// Precondition: a.size() == b.size().
[[nodiscard]] float dot(std::span<const float> a, std::span<const float> b) noexcept;

}  // namespace sentinel::simd
