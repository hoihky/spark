#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Result of a synchronous asset load (glTF, texture, mesh). */
template<typename T>
struct AssetLoadOutcome {
    bool ok = false;
    Utf8String path;
    Utf8String errorMessage;
    T value{};
};

}  // namespace Spark
