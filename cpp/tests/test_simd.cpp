#include <gtest/gtest.h>

#include <cmath>
#include <array>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

#include "sentinel/simd.hpp"

namespace {

struct Vectors {
    std::vector<float> a;
    std::vector<float> b;
};

Vectors random_vectors(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    Vectors v{std::vector<float>(n), std::vector<float>(n)};
    for (std::size_t i = 0; i < n; ++i) {
        v.a[i] = dist(rng);
        v.b[i] = dist(rng);
    }
    return v;
}

// Double-precision reference plus a rounding-error bound. A float32 dot
// product's error grows with n * eps * sum(|a_i * b_i|), so a fixed absolute
// tolerance would be either too loose for small n or too strict for large n.
struct Reference {
    double value;
    double tolerance;
};

Reference reference_dot(const Vectors& v) {
    double sum = 0.0;
    double abs_sum = 0.0;
    for (std::size_t i = 0; i < v.a.size(); ++i) {
        const double p = static_cast<double>(v.a[i]) * static_cast<double>(v.b[i]);
        sum += p;
        abs_sum += std::fabs(p);
    }
    const double eps = static_cast<double>(std::numeric_limits<float>::epsilon());
    const double n = static_cast<double>(v.a.size());
    return {sum, (n + 1.0) * eps * abs_sum + 1e-7};
}

class DotProduct : public ::testing::TestWithParam<std::size_t> {};

// Sizes chosen to hit every code path: empty, scalar tail only, exactly one
// 4-wide step, around the 16-wide unrolled boundary, and large.
constexpr std::array<std::size_t, 12> kSizes{0, 1, 3, 4, 5, 15, 16, 17, 31, 33, 1000, 1u << 20};

}  // namespace

INSTANTIATE_TEST_SUITE_P(Sizes, DotProduct, ::testing::ValuesIn(kSizes));

TEST_P(DotProduct, ScalarMatchesReference) {
    const auto v = random_vectors(GetParam(), 42);
    const auto ref = reference_dot(v);
    EXPECT_NEAR(sentinel::simd::dot_scalar(v.a, v.b), ref.value, ref.tolerance);
}

TEST_P(DotProduct, SimdMatchesReference) {
    const auto v = random_vectors(GetParam(), 42);
    const auto ref = reference_dot(v);
    EXPECT_NEAR(sentinel::simd::dot(v.a, v.b), ref.value, ref.tolerance);
}

TEST(DotProductExact, SmallIntegersAreExact) {
    // Small integers are exactly representable, so any summation order is exact.
    const std::vector<float> a{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    const std::vector<float> b(a.size(), 2.0f);
    EXPECT_EQ(sentinel::simd::dot(a, b), 306.0f);
    EXPECT_EQ(sentinel::simd::dot_scalar(a, b), 306.0f);
}
