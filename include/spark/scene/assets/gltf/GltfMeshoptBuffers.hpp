#pragma once

#include "spark/core/Utf8String.hpp"

struct cgltf_data;

namespace Spark {

/** Decompresses meshopt-compressed buffer views in-place (sets <c>buffer_view->data</c>). */
class GltfMeshoptBuffers {
public:
    [[nodiscard]] static bool TryDecodeAll(cgltf_data* data, Utf8String& outErrorMessage) noexcept;
};

}  // namespace Spark
