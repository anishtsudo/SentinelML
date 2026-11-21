#include "sentinel/build_info.hpp"

#include "sentinel/simd.hpp"

#ifndef SENTINEL_BUILD_TYPE
#define SENTINEL_BUILD_TYPE "unknown"
#endif
#ifndef SENTINEL_VERSION
#define SENTINEL_VERSION "unknown"
#endif

namespace sentinel {

BuildInfo build_info() noexcept {
    return BuildInfo{
        .version = SENTINEL_VERSION,
        .build_type = SENTINEL_BUILD_TYPE,
#if defined(__clang__)
        .compiler = __VERSION__,
#else
        .compiler = "non-clang",
#endif
        .cpp_standard = __cplusplus,
        .neon = simd::neon_enabled(),
    };
}

}  // namespace sentinel
