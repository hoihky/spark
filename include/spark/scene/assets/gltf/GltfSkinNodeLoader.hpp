#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"

struct cgltf_data;
struct cgltf_node;

namespace Spark {

class Skeleton;
class SkinnedMesh;

struct GltfSkinNodeBuildResult {
    bool ok = false;
    Utf8String errorMessage;
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
};

/** Builds one skinned mesh + skeleton from a glTF node that has mesh and skin. */
[[nodiscard]] GltfSkinNodeBuildResult TryBuildSkinNode(
        const cgltf_data* data,
        cgltf_node* skinNode,
        const char* sourcePath) noexcept;

}  // namespace Spark
