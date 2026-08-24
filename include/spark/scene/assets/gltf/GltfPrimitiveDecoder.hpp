#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <cstdint>

struct cgltf_attribute;
struct cgltf_data;
struct cgltf_primitive;

namespace Spark {

/** Four bone influences per vertex (glTF JOINTS_0 / WEIGHTS_0). */
struct GltfDecodedSkinning {
    std::uint32_t joints[4]{0, 0, 0, 0};
    float weights[4]{1.0F, 0.0F, 0.0F, 0.0F};
};

/** CPU attributes decoded from one glTF triangle primitive (local space). */
struct GltfDecodedPrimitive {
    Array<Vector3> positions;
    Array<Vector3> normals;
    Array<Vector2> texcoords;
    Array<Vector4> tangents;
    Array<GltfDecodedSkinning> skinning;
    /** Empty means non-indexed triangle list (0, 1, 2, …). */
    Array<std::uint32_t> indices;

    [[nodiscard]] bool HasSkinning() const noexcept { return !skinning.IsEmpty(); }
};

struct GltfPrimitiveDecodeResult {
    bool ok = false;
    Utf8String errorMessage;
    GltfDecodedPrimitive primitive;
};

struct GltfPrimitiveDecodeOptions {
    std::uint32_t texCoordSet = 0;
};

/** Strategy for turning a glTF primitive into decoded vertex/index arrays. */
class IGltfPrimitiveDecoder {
public:
    virtual ~IGltfPrimitiveDecoder() = default;

    [[nodiscard]] virtual bool Supports(const cgltf_primitive* prim) const noexcept = 0;

    [[nodiscard]] virtual GltfPrimitiveDecodeResult Decode(
            const cgltf_data* data,
            const cgltf_primitive* prim,
            const GltfPrimitiveDecodeOptions& options) const = 0;
};

/** Resolves the first decoder that supports <c>prim</c> and decodes it. */
class GltfPrimitiveDecoderRegistry {
public:
    [[nodiscard]] static GltfPrimitiveDecodeResult Decode(
            const cgltf_data* data,
            const cgltf_primitive* prim,
            const GltfPrimitiveDecodeOptions& options = {});
};

/** Draco unique attribute id stored by cgltf in <c>attribute.data</c> pointer index. */
[[nodiscard]] std::int32_t GltfDracoAttributeUniqueId(
        const cgltf_data* data,
        const cgltf_attribute& attr) noexcept;

}  // namespace Spark
