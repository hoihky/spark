#pragma once

#include "spark/math/Transform.hpp"

struct cgltf_node;

namespace Spark {

/** Converts glTF node local TRS / matrix into engine <c>Transform</c>. */
class GltfNodeTransforms {
public:
    [[nodiscard]] static Transform LocalTransform(const cgltf_node* node) noexcept;
};

}  // namespace Spark
