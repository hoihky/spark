# `spark/scene/spatial`

Reusable acceleration structures for axis-aligned bounds:

| Class | Use |
|-------|-----|
| `BvhTree` | Binary BVH (longest-axis or cyclic BSP splits), cached `BuildIfChanged` |
| `OctTree` | 3D octree with single-child placement when a cell fully contains an item |
| `QuadTree` | XZ quadtree; tall Y slab for cell vs frustum |
| `SpatialBoundsUtil` | Padded scene AABB union, bounds fingerprinting |

Frustum queries call `QueryFrustum(frustum, boundsArray, onIndex)` with conservative per-leaf tests.

Primary consumer: `SceneSpatialCull.cpp` (`ScenePartitionKind::*`).