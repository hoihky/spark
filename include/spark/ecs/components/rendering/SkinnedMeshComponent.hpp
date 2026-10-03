#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class Skeleton;
class SkinnedMesh;

/** References GPU-uploaded skinned geometry (Custom slot); pair with optional Skeleton and/or AnimatorComponent. */
class SkinnedMeshComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SkinnedMesh;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SkinnedMeshComponent(SharedPtr<SkinnedMesh> inMesh);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] const SharedPtr<SkinnedMesh>& GetMesh() const noexcept { return mesh; }
    void SetMesh(SharedPtr<SkinnedMesh> m);

    SPARK_SCRIPT_BIND(is_mesh_loaded)
    [[nodiscard]] bool IsMeshLoaded() const noexcept { return static_cast<bool>(mesh); }

    /** Rest/bind skeleton for palette when no <c>AnimatorComponent</c> is present. */
    [[nodiscard]] const SharedPtr<Skeleton>& GetSkeleton() const noexcept { return skeleton; }
    void SetSkeleton(SharedPtr<Skeleton> sk);

    SPARK_SCRIPT_BIND(is_skeleton_loaded)
    [[nodiscard]] bool IsSkeletonLoaded() const noexcept { return static_cast<bool>(skeleton); }

private:
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
};

}  // namespace Spark
