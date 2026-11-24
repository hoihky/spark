#pragma once

#include "spark/core/SharedPtr.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <cstdint>

namespace Spark {

/**
 * One discrete LOD step for {@link MeshLodComponent}.
 * <c>minDistance</c> is the camera distance (meters) at which this LOD becomes eligible.
 * Levels must be sorted by ascending <c>minDistance</c> (LOD0 at 0).
 */
struct MeshLodLevel {
    float minDistance = 0.0F;
    SceneMeshSlot meshSlot = SceneMeshSlot::Custom;
    SharedPtr<Mesh> rigidMesh{};
    SharedPtr<SkinnedMesh> skinnedMesh{};
    /** When true, nothing is submitted at this LOD (streaming / impostor hook). */
    bool culled = false;
    /**
     * Palette refresh stride for skinned draws (M7 animation LOD): 1 = every submit frame, 2 = every other, etc.
     * Only applies when a level supplies <c>skinnedMesh</c> or falls back to the owner's skinned asset.
     */
    std::uint32_t paletteUpdateStride = 1U;
};

}  // namespace Spark