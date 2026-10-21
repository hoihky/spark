#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class PhysicsBodyHandoff3D;

/**
 * Marks a dynamic rigidbody as kinematically carried by another object (typically a character).
 * Updated by <c>PhysicsBodyHandoff3D</c> before simulation.
 */
class PhysicsAttachedBody3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsAttachedBody3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] GameObject* GetCarrier() const noexcept { return carrier; }
    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return localOffset; }
    [[nodiscard]] bool IsAttached() const noexcept { return carrier != nullptr; }

private:
    friend class PhysicsBodyHandoff3D;

    void Bind(GameObject& carrierIn, const Vector3& offsetIn) noexcept;
    void ClearBinding() noexcept;

    GameObject* carrier = nullptr;
    Vector3 localOffset{Vector3::Zero};
};

}  // namespace Spark
