#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/physics/core/ColliderFilter.hpp"

#include <cstdint>

namespace Spark {

/**
 * Default collision layer for static 3D colliders on this object when shape components do not set their own bits.
 */
class PhysicsLayer3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsLayer3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return filter.categoryBits; }
    void SetCategoryBits(const std::uint16_t bits) noexcept { filter.categoryBits = bits; }

    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return filter.maskBits; }
    void SetMaskBits(const std::uint16_t bits) noexcept { filter.maskBits = bits; }

    [[nodiscard]] bool GetIsTrigger() const noexcept { return filter.isTrigger; }
    void SetIsTrigger(const bool value) noexcept { filter.isTrigger = value; }

    [[nodiscard]] const ColliderFilter& GetFilter() const noexcept { return filter; }

private:
    ColliderFilter filter{};
};

}  // namespace Spark
