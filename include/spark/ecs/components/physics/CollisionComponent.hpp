#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/** Simple sphere collider in local space; recenters when TransformChanged fires. */
class CollisionComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Collision;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit CollisionComponent(float sphereRadius = 0.5F, Vector3 localCenter = Vector3::Zero);

    void OnAttach(GameObject& owner) override;
    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    [[nodiscard]] const Vector3& GetLocalCenter() const noexcept { return localCenter; }

    /** Last world-space center after a transform signal (see RefreshWorldBounds). */
    [[nodiscard]] const Vector3& GetWorldCenter() const noexcept { return worldCenter; }

    void SetRadius(float r);
    void SetLocalCenter(const Vector3& c);

    void RefreshWorldBounds(GameObject& owner);

private:
    float radius = 0.5F;
    Vector3 localCenter{Vector3::Zero};
    Vector3 worldCenter{Vector3::Zero};
};

}  // namespace Spark
