#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Mesh.hpp"

namespace Spark {

/**
 * Drawable mesh + GPU slot + albedo. Listens for TransformChanged to react (e.g. rebuild bounds, LOD).
 */
class MeshComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Mesh;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    MeshComponent(SharedPtr<Mesh> inMesh, SceneMeshSlot inSlot, Vector3 inAlbedo);

    /** glTF / arbitrary mesh path: uses SceneMeshSlot::Custom for Vulkan dynamic geometry. */
    MeshComponent(SharedPtr<Mesh> inMesh, Vector3 inAlbedo);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] const SharedPtr<Mesh>& GetMesh() const noexcept { return mesh; }
    [[nodiscard]] SceneMeshSlot GetSlot() const noexcept { return slot; }
    [[nodiscard]] const Vector3& GetAlbedo() const noexcept { return albedo; }

    void SetMesh(SharedPtr<Mesh> m);
    void SetAlbedo(const Vector3& c);

private:
    SharedPtr<Mesh> mesh;
    SceneMeshSlot slot = SceneMeshSlot::UnitCube;
    Vector3 albedo{1.0F, 1.0F, 1.0F};
};

}  // namespace Spark
