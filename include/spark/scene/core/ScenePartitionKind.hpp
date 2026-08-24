#pragma once

#include <cstdint>

namespace Spark {

/**
 * Scene-space acceleration used when traversing drawables against the view frustum.
 * None keeps legacy behavior (no frustum rejection). Other modes still perform a conservative
 * AABB vs frustum test at leaves so large meshes are not dropped incorrectly.
 */
enum class ScenePartitionKind : std::uint8_t {
    /** No frustum culling; visit every drawable (same cost profile as Scene::ForEachDrawable). */
    None = 0,
    /** Linear scan with per-object world AABB vs frustum. */
    BruteForce,
    /** Axis-aligned BSP / kd-tree on object centroids for hierarchy; leaves test full AABB. */
    AxisAlignedBinarySpacePartition,
    /** Recursive axis-aligned octants; objects may be referenced from multiple overlapping cells. */
    Octree,
    /** Vertical (world Y) quadtree on XZ footprint; frustum test uses full 3D AABB at leaves. */
    QuadTree,
    /** Bounding volume hierarchy (binary) built from object AABBs. */
    BoundingVolumeHierarchy,
};

}  // namespace Spark
