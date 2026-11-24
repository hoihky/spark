#include "spark/scene/core/SceneDrawableFrustumSink.hpp"

#include "spark/core/Array.hpp"
#include "spark/scene/spatial/BvhTree.hpp"
#include "spark/scene/spatial/OctTree.hpp"
#include "spark/scene/spatial/QuadTree.hpp"
#include "spark/scene/spatial/SpatialBoundsUtil.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

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

[[nodiscard]] bool TryBuiltinMeshLocalBounds(SceneMeshSlot slot, Vector3& outMin, Vector3& outMax) noexcept {
    switch (slot) {
    case SceneMeshSlot::UnitCube:
        outMin = Vector3{-0.5F, -0.5F, -0.5F};
        outMax = Vector3{0.5F, 0.5F, 0.5F};
        return true;
    case SceneMeshSlot::GroundPlane:
        outMin = Vector3{-0.5F, 0.0F, -0.5F};
        outMax = Vector3{0.5F, 0.0F, 0.5F};
        return true;
    case SceneMeshSlot::Custom:
        break;
    }
    return false;
}

[[nodiscard]] bool TryWorldAabbFromMesh(const MeshComponent& mc, const Matrix4& world, Vector3& outMin, Vector3& outMax) {
    Vector3 lm;
    Vector3 lx;
    if (!mc.GetMesh()) {
        if (!TryBuiltinMeshLocalBounds(mc.GetSlot(), lm, lx)) {
            return false;
        }
    } else if (!mc.GetMesh()->TryComputeAxisAlignedBounds(lm, lx)) {
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
    scene.GetWorld().Entities().ForEachActive([&out](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (o->GetComponent<WaterBodyComponent>() != nullptr) {
            return;
        }
        const MeshComponent* mc = o->GetComponent<MeshComponent>();
        if (mc == nullptr) {
            return;
        }
        if (!mc->GetMesh() && mc->GetSlot() == SceneMeshSlot::Custom) {
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

void GatherWaterBodies(const Scene& scene, Array<DrawableRecord>& out) {
    out.Clear();
    scene.GetWorld().Entities().ForEachActive([&out](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (o->GetComponent<WaterBodyComponent>() == nullptr) {
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
    world.Entities().ForEachActive([&out](GameObject* o) {
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

void GatherDrawableBounds(const Array<DrawableRecord>& records, Array<AxisAlignedBox>& outBounds) {
    outBounds.Clear();
    outBounds.Reserve(records.GetSize());
    for (std::size_t i = 0; i < records.GetSize(); ++i) {
        const DrawableRecord& r = records[i];
        outBounds.PushBack(AxisAlignedBox{r.aabbMin, r.aabbMax});
    }
}

void EmitDrawable(DrawableFrustumSink& sink, const Array<DrawableRecord>& records, int index) {
    const DrawableRecord& r = records[static_cast<std::size_t>(index)];
    sink.OnDrawable(r.object, *r.mesh, r.material, r.world);
}

[[nodiscard]] std::uint64_t FingerprintDrawableRecords(const Array<DrawableRecord>& records) noexcept {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](const std::uint64_t v) noexcept {
        h ^= v;
        h *= 1099511628211ULL;
    };
    mix(static_cast<std::uint64_t>(records.GetSize()));
    for (std::size_t i = 0; i < records.GetSize(); ++i) {
        const DrawableRecord& r = records[i];
        const std::uint64_t* minWords = reinterpret_cast<const std::uint64_t*>(&r.aabbMin.x);
        const std::uint64_t* maxWords = reinterpret_cast<const std::uint64_t*>(&r.aabbMax.x);
        mix(reinterpret_cast<std::uintptr_t>(r.object));
        mix(minWords[0]);
        mix(minWords[1]);
        mix(maxWords[0]);
        mix(maxWords[1]);
    }
    return h;
}

void RunDrawableCull(
        const Array<DrawableRecord>& records, const Frustum& frustum, ScenePartitionKind mode, DrawableFrustumSink& sink) {
    if (records.IsEmpty()) {
        return;
    }
    Array<AxisAlignedBox> bounds;
    GatherDrawableBounds(records, bounds);
    const AxisAlignedBox* boundsPtr = bounds.GetData();
    const std::size_t count = bounds.GetSize();

    switch (mode) {
        case ScenePartitionKind::None:
            break;
        case ScenePartitionKind::BruteForce:
            BruteForceCull(records, frustum, sink);
            break;
        case ScenePartitionKind::BoundingVolumeHierarchy: {
            static BvhTree cache{};
            BvhTree::BuildOptions opts{};
            opts.maxLeafSize = 4;
            opts.maxDepth = 24;
            opts.splitPolicy = BvhTree::SplitPolicy::LongestExtent;
            cache.BuildIfChanged(boundsPtr, count, FingerprintDrawableRecords(records), opts);
            cache.QueryFrustum(frustum, boundsPtr, [&](const int idx) { EmitDrawable(sink, records, idx); });
            break;
        }
        case ScenePartitionKind::AxisAlignedBinarySpacePartition: {
            BvhTree bsp{};
            BvhTree::BuildOptions opts{};
            opts.maxLeafSize = 6;
            opts.maxDepth = 28;
            opts.splitPolicy = BvhTree::SplitPolicy::CyclicByDepth;
            bsp.Build(boundsPtr, count, opts);
            bsp.QueryFrustum(frustum, boundsPtr, [&](const int idx) { EmitDrawable(sink, records, idx); });
            break;
        }
        case ScenePartitionKind::Octree: {
            OctTree oct{};
            OctTree::Options octOpts{};
            octOpts.maxDepth = 14;
            octOpts.maxLeafSize = 8;
            oct.Reset(SpatialBoundsUtil::UnionSceneItemsPadded(boundsPtr, count), octOpts);
            for (std::size_t i = 0; i < count; ++i) {
                oct.Insert(static_cast<int>(i), boundsPtr, count);
            }
            oct.QueryFrustum(frustum, boundsPtr, [&](const int idx) { EmitDrawable(sink, records, idx); });
            break;
        }
        case ScenePartitionKind::QuadTree: {
            const AxisAlignedBox root = SpatialBoundsUtil::UnionSceneItemsPadded(boundsPtr, count);
            QuadTree quad{};
            QuadTree::Options quadOpts{};
            quadOpts.maxDepth = 16;
            quadOpts.maxLeafSize = 10;
            quad.Reset(root.min.x, root.max.x, root.min.z, root.max.z, quadOpts);
            for (std::size_t i = 0; i < count; ++i) {
                quad.Insert(static_cast<int>(i), boundsPtr, count);
            }
            quad.QueryFrustum(frustum, boundsPtr, [&](const int idx) { EmitDrawable(sink, records, idx); });
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
        scene.GetWorld().Entities().ForEachActive([&sink](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            if (o->GetComponent<WaterBodyComponent>() != nullptr) {
                return;
            }
            const MeshComponent* mc = o->GetComponent<MeshComponent>();
            if (mc == nullptr) {
                return;
            }
            if (!mc->GetMesh() && mc->GetSlot() == SceneMeshSlot::Custom) {
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
        world.Entities().ForEachActive([&sink](GameObject* o) {
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

void DispatchWaterBodyFrustumCull(
        const Scene& scene, const Matrix4& viewProjection, ScenePartitionKind mode, DrawableFrustumSink& sink) {
    if (mode == ScenePartitionKind::None) {
        scene.ForEachWaterBody([&sink](GameObject& o,
                                         const WaterBodyComponent& /*water*/,
                                         const MeshComponent& mesh,
                                         const MaterialComponent* mat,
                                         const Matrix4& worldM) {
            sink.OnDrawable(&o, mesh, mat, worldM);
        });
        return;
    }
    Array<DrawableRecord> records;
    records.Reserve(16);
    GatherWaterBodies(scene, records);
    const Frustum fr = Frustum::FromColumnMajorViewProjection(viewProjection);
    RunDrawableCull(records, fr, mode, sink);
}

}  // namespace Spark
