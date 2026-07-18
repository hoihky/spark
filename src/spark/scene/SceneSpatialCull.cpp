#include "spark/scene/SceneDrawableFrustumSink.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/scene/SkinnedMesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Spark {

namespace {

struct DrawableRecord {
    GameObject* object = nullptr;
    const MeshComponent* mesh = nullptr;
    const MaterialComponent* material = nullptr;
    Matrix4 world{};
    Vector3 aabbMin{};
    Vector3 aabbMax{};
};

struct SkinnedRecord {
    GameObject* object = nullptr;
    const SkinnedMeshComponent* skinned = nullptr;
    const MaterialComponent* material = nullptr;
    const AnimatorComponent* animator = nullptr;
    Matrix4 world{};
    Vector3 aabbMin{};
    Vector3 aabbMax{};
};

[[nodiscard]] bool TryWorldAabbFromMesh(const MeshComponent& mc, const Matrix4& world, Vector3& outMin, Vector3& outMax) {
    if (!mc.GetMesh()) {
        return false;
    }
    Vector3 lm;
    Vector3 lx;
    if (!mc.GetMesh()->TryComputeAxisAlignedBounds(lm, lx)) {
        lm = Vector3{-1.0F, -1.0F, -1.0F};
        lx = Vector3{1.0F, 1.0F, 1.0F};
    }
    AxisAlignedBox::EncapsulateTransformedLocal(lm, lx, world, outMin, outMax);
    return true;
}

[[nodiscard]] bool TryWorldAabbFromSkinned(const SkinnedMeshComponent& smc, const Matrix4& world, Vector3& outMin, Vector3& outMax) {
    if (!smc.GetMesh()) {
        return false;
    }
    const Array<SkinnedMesh::Vertex>& v = smc.GetMesh()->GetVertices();
    if (v.IsEmpty()) {
        return false;
    }
    Vector3 lm = v[0].position;
    Vector3 lx = v[0].position;
    for (std::size_t i = 1; i < v.GetSize(); ++i) {
        const Vector3& p = v[i].position;
        lm.x = std::min(lm.x, p.x);
        lm.y = std::min(lm.y, p.y);
        lm.z = std::min(lm.z, p.z);
        lx.x = std::max(lx.x, p.x);
        lx.y = std::max(lx.y, p.y);
        lx.z = std::max(lx.z, p.z);
    }
    AxisAlignedBox::EncapsulateTransformedLocal(lm, lx, world, outMin, outMax);
    return true;
}

void GatherDrawables(const Scene& scene, Array<DrawableRecord>& out) {
    out.Clear();
    scene.GetWorld().ForEachGameObject([&out](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const MeshComponent* mc = o->GetComponent<MeshComponent>();
        if (mc == nullptr || !mc->GetMesh()) {
            return;
        }
        DrawableRecord r{};
        r.object = o;
        r.mesh = mc;
        r.material = o->GetComponent<MaterialComponent>();
        r.world = o->GetWorldMatrix();
        if (!TryWorldAabbFromMesh(*mc, r.world, r.aabbMin, r.aabbMax)) {
            return;
        }
        out.PushBack(r);
    });
}

void GatherSkinned(const GameWorld& world, Array<SkinnedRecord>& out) {
    out.Clear();
    world.ForEachGameObject([&out](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const SkinnedMeshComponent* smc = o->GetComponent<SkinnedMeshComponent>();
        if (smc == nullptr || !smc->GetMesh()) {
            return;
        }
        SkinnedRecord r{};
        r.object = o;
        r.skinned = smc;
        r.material = o->GetComponent<MaterialComponent>();
        r.animator = o->GetComponent<AnimatorComponent>();
        r.world = o->GetWorldMatrix();
        if (!TryWorldAabbFromSkinned(*smc, r.world, r.aabbMin, r.aabbMax)) {
            return;
        }
        out.PushBack(r);
    });
}

void BruteForceCull(const Array<DrawableRecord>& records, const Frustum& frustum, DrawableFrustumSink& sink) {
    for (std::size_t i = 0; i < records.GetSize(); ++i) {
        const DrawableRecord& r = records[i];
        if (frustum.IntersectsAxisAlignedBox(r.aabbMin, r.aabbMax)) {
            sink.OnDrawable(r.object, *r.mesh, r.material, r.world);
        }
    }
}

void BruteForceSkinnedCull(const Array<SkinnedRecord>& records, const Frustum& frustum, SkinnedDrawableFrustumSink& sink) {
    for (std::size_t i = 0; i < records.GetSize(); ++i) {
        const SkinnedRecord& r = records[i];
        if (frustum.IntersectsAxisAlignedBox(r.aabbMin, r.aabbMax)) {
            sink.OnSkinnedDrawable(r.object, *r.skinned, r.material, r.animator, r.world);
        }
    }
}

struct BvhNode {
    AxisAlignedBox bounds{};
    int left = -1;
    int right = -1;
    int first = 0;
    int count = 0;
};

[[nodiscard]] AxisAlignedBox BoundsOfRange(
        const Array<DrawableRecord>& recs, const std::vector<int>& ids, int first, int count) {
    AxisAlignedBox b{recs[static_cast<std::size_t>(ids[static_cast<std::size_t>(first)])].aabbMin,
            recs[static_cast<std::size_t>(ids[static_cast<std::size_t>(first)])].aabbMax};
    for (int i = 1; i < count; ++i) {
        const DrawableRecord& r = recs[static_cast<std::size_t>(ids[static_cast<std::size_t>(first + i)])];
        b = AxisAlignedBox::UnionOf(b, AxisAlignedBox{r.aabbMin, r.aabbMax});
    }
    return b;
}

[[nodiscard]] Vector3 Centroid(const DrawableRecord& r) {
    return (r.aabbMin + r.aabbMax) * 0.5F;
}

int BuildBvhRecursive(const Array<DrawableRecord>& recs,
        std::vector<int>& ids,
        int first,
        int count,
        int depth,
        int axisMode,
        std::vector<BvhNode>& nodes,
        int maxLeaf,
        int maxDepth) {
    const int nodeIndex = static_cast<int>(nodes.size());
    BvhNode node{};
    node.first = first;
    node.count = count;
    node.bounds = BoundsOfRange(recs, ids, first, count);
    nodes.push_back(node);

    if (count <= maxLeaf || depth >= maxDepth) {
        return nodeIndex;
    }

    int axis = 0;
    if (axisMode == 0) {
        const Vector3 e = node.bounds.max - node.bounds.min;
        axis = (e.x >= e.y && e.x >= e.z) ? 0 : (e.y >= e.z ? 1 : 2);
    } else {
        axis = depth % 3;
    }

    const int begin = first;
    const int end = first + count;
    const int midOff = count / 2;
    std::nth_element(ids.begin() + begin, ids.begin() + begin + midOff, ids.begin() + end,
            [&recs, axis](int a, int b) {
                const Vector3 ca3 = Centroid(recs[static_cast<std::size_t>(a)]);
                const Vector3 cb3 = Centroid(recs[static_cast<std::size_t>(b)]);
                const float ca = axis == 0 ? ca3.x : (axis == 1 ? ca3.y : ca3.z);
                const float cb = axis == 0 ? cb3.x : (axis == 1 ? cb3.y : cb3.z);
                if (ca < cb) {
                    return true;
                }
                if (cb < ca) {
                    return false;
                }
                return a < b;
            });
    const int mid = first + count / 2;
    const int leftCount = mid - first;
    const int rightCount = count - leftCount;
    if (leftCount == 0 || rightCount == 0) {
        return nodeIndex;
    }

    const int L = BuildBvhRecursive(recs, ids, first, leftCount, depth + 1, axisMode, nodes, maxLeaf, maxDepth);
    const int R = BuildBvhRecursive(recs, ids, mid, rightCount, depth + 1, axisMode, nodes, maxLeaf, maxDepth);
    nodes[static_cast<std::size_t>(nodeIndex)].left = L;
    nodes[static_cast<std::size_t>(nodeIndex)].right = R;
    nodes[static_cast<std::size_t>(nodeIndex)].count = 0;
    return nodeIndex;
}

void QueryBvh(const Array<DrawableRecord>& recs,
        const std::vector<int>& ids,
        const std::vector<BvhNode>& nodes,
        int nodeIndex,
        const Frustum& frustum,
        DrawableFrustumSink& sink) {
    if (nodeIndex < 0) {
        return;
    }
    const BvhNode& n = nodes[static_cast<std::size_t>(nodeIndex)];
    if (!frustum.Intersects(n.bounds)) {
        return;
    }
    if (n.count > 0) {
        for (int i = 0; i < n.count; ++i) {
            const int pi = ids[static_cast<std::size_t>(n.first + i)];
            const DrawableRecord& r = recs[static_cast<std::size_t>(pi)];
            if (frustum.IntersectsAxisAlignedBox(r.aabbMin, r.aabbMax)) {
                sink.OnDrawable(r.object, *r.mesh, r.material, r.world);
            }
        }
        return;
    }
    QueryBvh(recs, ids, nodes, n.left, frustum, sink);
    QueryBvh(recs, ids, nodes, n.right, frustum, sink);
}

// --- Octree: single placement when a child cell fully contains the drawable AABB ---

struct OctNode {
    AxisAlignedBox cell{};
    std::vector<int> prims;
    std::array<int, 8> children{};
    OctNode() { children.fill(-1); }
};

[[nodiscard]] AxisAlignedBox OctantOf(const AxisAlignedBox& parent, int oct) {
    const Vector3 c = (parent.min + parent.max) * 0.5F;
    Vector3 mn = parent.min;
    Vector3 mx = parent.max;
    if ((oct & 1) != 0) {
        mn.x = c.x;
    } else {
        mx.x = c.x;
    }
    if ((oct & 2) != 0) {
        mn.y = c.y;
    } else {
        mx.y = c.y;
    }
    if ((oct & 4) != 0) {
        mn.z = c.z;
    } else {
        mx.z = c.z;
    }
    return AxisAlignedBox{mn, mx};
}

[[nodiscard]] bool CellFullyContains(const AxisAlignedBox& cell, const Vector3& amin, const Vector3& amax) noexcept {
    return amin.x >= cell.min.x && amax.x <= cell.max.x && amin.y >= cell.min.y && amax.y <= cell.max.y &&
            amin.z >= cell.min.z && amax.z <= cell.max.z;
}

void OctInsert(std::vector<OctNode>& nodes,
        int nodeIndex,
        int prim,
        const Array<DrawableRecord>& recs,
        int depth,
        int maxDepth,
        int maxLeaf) {
    OctNode& n = nodes[static_cast<std::size_t>(nodeIndex)];
    const DrawableRecord& r = recs[static_cast<std::size_t>(prim)];

    if (n.children[0] < 0 && (depth >= maxDepth || static_cast<int>(n.prims.size()) < maxLeaf)) {
        n.prims.push_back(prim);
        return;
    }

    if (n.children[0] < 0) {
        const std::vector<int> old = std::move(n.prims);
        n.prims.clear();
        for (int o = 0; o < 8; ++o) {
            OctNode child{};
            child.cell = OctantOf(n.cell, o);
            n.children[static_cast<std::size_t>(o)] = static_cast<int>(nodes.size());
            nodes.push_back(child);
        }
        for (int p : old) {
            OctInsert(nodes, nodeIndex, p, recs, depth, maxDepth, maxLeaf);
        }
    }

    int targetChild = -1;
    if (depth < maxDepth) {
        for (int o = 0; o < 8; ++o) {
            const int ci = n.children[static_cast<std::size_t>(o)];
            if (CellFullyContains(nodes[static_cast<std::size_t>(ci)].cell, r.aabbMin, r.aabbMax)) {
                targetChild = ci;
                break;
            }
        }
    }
    if (targetChild >= 0) {
        OctInsert(nodes, targetChild, prim, recs, depth + 1, maxDepth, maxLeaf);
    } else {
        n.prims.push_back(prim);
    }
}

void OctQuery(const std::vector<OctNode>& nodes,
        int nodeIndex,
        const Frustum& frustum,
        const Array<DrawableRecord>& recs,
        DrawableFrustumSink& sink) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size())) {
        return;
    }
    const OctNode& n = nodes[static_cast<std::size_t>(nodeIndex)];
    if (!frustum.Intersects(n.cell)) {
        return;
    }
    for (int pi : n.prims) {
        const DrawableRecord& r = recs[static_cast<std::size_t>(pi)];
        if (frustum.IntersectsAxisAlignedBox(r.aabbMin, r.aabbMax)) {
            sink.OnDrawable(r.object, *r.mesh, r.material, r.world);
        }
    }
    for (int o = 0; o < 8; ++o) {
        const int ci = n.children[static_cast<std::size_t>(o)];
        if (ci >= 0) {
            OctQuery(nodes, ci, frustum, recs, sink);
        }
    }
}

// --- Quadtree on XZ (world vertical = Y); 3D frustum test at leaves ---

struct QuadNode {
    float minX = 0.0F;
    float maxX = 0.0F;
    float minZ = 0.0F;
    float maxZ = 0.0F;
    std::vector<int> prims;
    std::array<int, 4> children{};
    QuadNode() { children.fill(-1); }
};

[[nodiscard]] bool QuadCellFullyContains(float minX, float maxX, float minZ, float maxZ, const Vector3& amin, const Vector3& amax) noexcept {
    return amin.x >= minX && amax.x <= maxX && amin.z >= minZ && amax.z <= maxZ;
}

void QuadChildBounds(
        float pminX, float pmaxX, float pminZ, float pmaxZ, int quadrant, float& cminX, float& cmaxX, float& cminZ, float& cmaxZ) {
    const float midX = (pminX + pmaxX) * 0.5F;
    const float midZ = (pminZ + pmaxZ) * 0.5F;
    switch (quadrant) {
        case 0:
            cminX = pminX;
            cmaxX = midX;
            cminZ = pminZ;
            cmaxZ = midZ;
            break;
        case 1:
            cminX = midX;
            cmaxX = pmaxX;
            cminZ = pminZ;
            cmaxZ = midZ;
            break;
        case 2:
            cminX = pminX;
            cmaxX = midX;
            cminZ = midZ;
            cmaxZ = pmaxZ;
            break;
        default:
            cminX = midX;
            cmaxX = pmaxX;
            cminZ = midZ;
            cmaxZ = pmaxZ;
            break;
    }
}

void QuadInsert(std::vector<QuadNode>& nodes,
        int nodeIndex,
        int prim,
        const Array<DrawableRecord>& recs,
        int depth,
        int maxDepth,
        int maxLeaf) {
    QuadNode& n = nodes[static_cast<std::size_t>(nodeIndex)];
    const DrawableRecord& r = recs[static_cast<std::size_t>(prim)];

    if (n.children[0] < 0 && (depth >= maxDepth || static_cast<int>(n.prims.size()) < maxLeaf)) {
        n.prims.push_back(prim);
        return;
    }

    if (n.children[0] < 0) {
        const std::vector<int> old = std::move(n.prims);
        n.prims.clear();
        for (int q = 0; q < 4; ++q) {
            QuadNode child{};
            QuadChildBounds(n.minX, n.maxX, n.minZ, n.maxZ, q, child.minX, child.maxX, child.minZ, child.maxZ);
            n.children[static_cast<std::size_t>(q)] = static_cast<int>(nodes.size());
            nodes.push_back(child);
        }
        for (int p : old) {
            QuadInsert(nodes, nodeIndex, p, recs, depth, maxDepth, maxLeaf);
        }
    }

    int target = -1;
    if (depth < maxDepth) {
        for (int q = 0; q < 4; ++q) {
            const int ci = n.children[static_cast<std::size_t>(q)];
            const QuadNode& ch = nodes[static_cast<std::size_t>(ci)];
            if (QuadCellFullyContains(ch.minX, ch.maxX, ch.minZ, ch.maxZ, r.aabbMin, r.aabbMax)) {
                target = ci;
                break;
            }
        }
    }
    if (target >= 0) {
        QuadInsert(nodes, target, prim, recs, depth + 1, maxDepth, maxLeaf);
    } else {
        n.prims.push_back(prim);
    }
}

void QuadQuery(const std::vector<QuadNode>& nodes,
        int nodeIndex,
        const Frustum& frustum,
        const Array<DrawableRecord>& recs,
        DrawableFrustumSink& sink) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size())) {
        return;
    }
    const QuadNode& n = nodes[static_cast<std::size_t>(nodeIndex)];
    const Vector3 qmn{n.minX, -1.0e9F, n.minZ};
    const Vector3 qmx{n.maxX, 1.0e9F, n.maxZ};
    if (!frustum.IntersectsAxisAlignedBox(qmn, qmx)) {
        return;
    }
    for (int pi : n.prims) {
        const DrawableRecord& r = recs[static_cast<std::size_t>(pi)];
        if (frustum.IntersectsAxisAlignedBox(r.aabbMin, r.aabbMax)) {
            sink.OnDrawable(r.object, *r.mesh, r.material, r.world);
        }
    }
    for (int q = 0; q < 4; ++q) {
        const int ci = n.children[static_cast<std::size_t>(q)];
        if (ci >= 0) {
            QuadQuery(nodes, ci, frustum, recs, sink);
        }
    }
}

[[nodiscard]] AxisAlignedBox UnionSceneBounds(const Array<DrawableRecord>& recs) {
    if (recs.IsEmpty()) {
        return AxisAlignedBox{Vector3::Zero, Vector3::Zero};
    }
    AxisAlignedBox b{recs[0].aabbMin, recs[0].aabbMax};
    for (std::size_t i = 1; i < recs.GetSize(); ++i) {
        b = AxisAlignedBox::UnionOf(b, AxisAlignedBox{recs[i].aabbMin, recs[i].aabbMax});
    }
    const Vector3 pad = (b.max - b.min) * 0.02F + Vector3{0.25F, 0.25F, 0.25F};
    return AxisAlignedBox{b.min - pad, b.max + pad};
}

void RunDrawableCull(
        const Array<DrawableRecord>& records, const Frustum& frustum, ScenePartitionKind mode, DrawableFrustumSink& sink) {
    if (records.IsEmpty()) {
        return;
    }
    switch (mode) {
        case ScenePartitionKind::None:
            break;
        case ScenePartitionKind::BruteForce:
            BruteForceCull(records, frustum, sink);
            break;
        case ScenePartitionKind::BoundingVolumeHierarchy: {
            std::vector<int> ids(records.GetSize());
            for (std::size_t i = 0; i < records.GetSize(); ++i) {
                ids[i] = static_cast<int>(i);
            }
            std::vector<BvhNode> nodes;
            nodes.reserve(records.GetSize() * 2);
            BuildBvhRecursive(records, ids, 0, static_cast<int>(records.GetSize()), 0, 0, nodes, 4, 24);
            QueryBvh(records, ids, nodes, 0, frustum, sink);
            break;
        }
        case ScenePartitionKind::AxisAlignedBinarySpacePartition: {
            std::vector<int> ids(records.GetSize());
            for (std::size_t i = 0; i < records.GetSize(); ++i) {
                ids[i] = static_cast<int>(i);
            }
            std::vector<BvhNode> nodes;
            nodes.reserve(records.GetSize() * 2);
            BuildBvhRecursive(records, ids, 0, static_cast<int>(records.GetSize()), 0, 1, nodes, 6, 28);
            QueryBvh(records, ids, nodes, 0, frustum, sink);
            break;
        }
        case ScenePartitionKind::Octree: {
            const AxisAlignedBox rootBox = UnionSceneBounds(records);
            std::vector<OctNode> oct;
            OctNode root{};
            root.cell = rootBox;
            oct.push_back(root);
            for (std::size_t i = 0; i < records.GetSize(); ++i) {
                OctInsert(oct, 0, static_cast<int>(i), records, 0, 14, 8);
            }
            OctQuery(oct, 0, frustum, records, sink);
            break;
        }
        case ScenePartitionKind::QuadTree: {
            const AxisAlignedBox u = UnionSceneBounds(records);
            std::vector<QuadNode> qt;
            QuadNode root{};
            root.minX = u.min.x;
            root.maxX = u.max.x;
            root.minZ = u.min.z;
            root.maxZ = u.max.z;
            qt.push_back(root);
            for (std::size_t i = 0; i < records.GetSize(); ++i) {
                QuadInsert(qt, 0, static_cast<int>(i), records, 0, 16, 10);
            }
            QuadQuery(qt, 0, frustum, records, sink);
            break;
        }
    }
}

void RunSkinnedCull(
        const Array<SkinnedRecord>& records, const Frustum& frustum, ScenePartitionKind mode, SkinnedDrawableFrustumSink& sink) {
    (void)mode;
    BruteForceSkinnedCull(records, frustum, sink);
}

}  // namespace

void DispatchDrawableFrustumCull(
        const Scene& scene, const Matrix4& viewProjection, ScenePartitionKind mode, DrawableFrustumSink& sink) {
    if (mode == ScenePartitionKind::None) {
        scene.GetWorld().ForEachGameObject([&sink](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const MeshComponent* mc = o->GetComponent<MeshComponent>();
            if (mc == nullptr || !mc->GetMesh()) {
                return;
            }
            const MaterialComponent* mat = o->GetComponent<MaterialComponent>();
            sink.OnDrawable(o, *mc, mat, o->GetWorldMatrix());
        });
        return;
    }
    Array<DrawableRecord> records;
    records.Reserve(256);
    GatherDrawables(scene, records);
    const Frustum fr = Frustum::FromColumnMajorViewProjection(viewProjection);
    RunDrawableCull(records, fr, mode, sink);
}

void DispatchSkinnedDrawableFrustumCull(
        const GameWorld& world,
        const Matrix4& viewProjection,
        ScenePartitionKind mode,
        SkinnedDrawableFrustumSink& sink) {
    if (mode == ScenePartitionKind::None) {
        world.ForEachGameObject([&sink](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const SkinnedMeshComponent* smc = o->GetComponent<SkinnedMeshComponent>();
            if (smc == nullptr || !smc->GetMesh()) {
                return;
            }
            const MaterialComponent* mat = o->GetComponent<MaterialComponent>();
            const AnimatorComponent* anim = o->GetComponent<AnimatorComponent>();
            sink.OnSkinnedDrawable(o, *smc, mat, anim, o->GetWorldMatrix());
        });
        return;
    }
    Array<SkinnedRecord> records;
    records.Reserve(64);
    GatherSkinned(world, records);
    const Frustum fr = Frustum::FromColumnMajorViewProjection(viewProjection);
    RunSkinnedCull(records, fr, mode, sink);
}

void DispatchSkinnedDrawableFrustumCull(const Scene& scene,
        const Matrix4& viewProjection,
        ScenePartitionKind mode,
        SkinnedDrawableFrustumSink& sink) {
    DispatchSkinnedDrawableFrustumCull(scene.GetWorld(), viewProjection, mode, sink);
}

}  // namespace Spark
