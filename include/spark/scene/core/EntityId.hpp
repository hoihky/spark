#pragma once

#include <cstdint>

namespace Spark {

/** Stable opaque id for a <c>GameObject</c> within a <c>GameWorld</c> (monotonic; not reused after destroy). */
using EntityId = std::uint64_t;

inline constexpr EntityId kInvalidEntityId = 0;

}  // namespace Spark
