#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/** FNV-1a 64-bit — used for GPU upload dirty checks (textures, mesh blobs). */
[[nodiscard]] inline std::uint64_t Fnv64Begin() noexcept {
    return 1469598103934665603ull;
}

inline void Fnv64Mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ull;
}

inline std::uint64_t Fnv64HashBytes(std::uint64_t hash, const void* data, std::size_t byteCount) noexcept {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < byteCount; ++i) {
        Fnv64Mix(hash, static_cast<std::uint64_t>(bytes[i]));
    }
    return hash;
}

}  // namespace Spark
