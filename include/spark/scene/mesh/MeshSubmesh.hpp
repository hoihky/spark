#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>

namespace Spark {

/** Maps a glTF material-variant index to a material slot. */
struct MeshSubmeshVariantMapping {
    std::uint32_t variantIndex = 0;
    std::uint32_t materialIndex = 0;
};

/** Index range + glTF material index for one primitive within a shared vertex buffer. */
struct MeshSubmesh {
    std::uint32_t indexOffset = 0;
    std::uint32_t indexCount = 0;
    /** Default material when no variant mapping matches. */
    std::uint32_t materialIndex = 0;
    /** Non-empty when the primitive uses KHR_materials_variants. */
    Array<MeshSubmeshVariantMapping> variantMappings;

    [[nodiscard]] std::uint32_t ResolveMaterialIndex(const std::uint32_t activeVariant) const noexcept {
        for (std::size_t i = 0; i < variantMappings.GetSize(); ++i) {
            if (variantMappings[i].variantIndex == activeVariant) {
                return variantMappings[i].materialIndex;
            }
        }
        return materialIndex;
    }
};

/** SceneDrawItem::submeshIndex value meaning "draw the full mesh index buffer". */
constexpr std::uint32_t kSceneDrawFullSubmesh = UINT32_MAX;

}  // namespace Spark
