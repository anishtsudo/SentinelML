#include <gtest/gtest.h>

#include "sentinel/build_info.hpp"

TEST(BuildInfo, ReportsCpp20OrLater) {
    EXPECT_GE(sentinel::build_info().cpp_standard, 202002L);
}

TEST(BuildInfo, BuildTypeIsKnown) {
    EXPECT_NE(sentinel::build_info().build_type, "unknown");
}

#if defined(__aarch64__) || defined(__arm64__)
TEST(BuildInfo, NeonEnabledOnArm64) {
    EXPECT_TRUE(sentinel::build_info().neon);
}
#endif
