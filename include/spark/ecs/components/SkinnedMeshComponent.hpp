#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class SkinnedMesh;

/** References GPU-uploaded skinned geometry (Custom slot); pair with AnimatorComponent + MaterialComponent. */
class SkinnedMeshComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SkinnedMesh;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SkinnedMeshComponent(SharedPtr<SkinnedMesh> inMesh);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] const SharedPtr<SkinnedMesh>& GetMesh() const noexcept { return mesh; }
    void SetMesh(SharedPtr<SkinnedMesh> m);

private:
    SharedPtr<SkinnedMesh> mesh;
};

}  // namespace Spark
