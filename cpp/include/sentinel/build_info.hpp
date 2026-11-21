#pragma once

#include <string_view>

namespace sentinel {

// Compile-time facts about this build. Benchmark output embeds these so a
// result can never be silently attributed to the wrong build (e.g. Debug).
struct BuildInfo {
    std::string_view version;
    std::string_view build_type;  // "Release", "Debug", ...
    std::string_view compiler;
    long cpp_standard;            // value of __cplusplus
    bool neon;                    // compiled with ARM NEON intrinsics enabled
};

[[nodiscard]] BuildInfo build_info() noexcept;

}  // namespace sentinel
