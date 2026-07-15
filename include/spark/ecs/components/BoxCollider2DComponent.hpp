#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/**
 * Axis-aligned box in local XY (Z ignored), aligned with the standard sprite quad (local ±0.5 before scale).
 * World AABB uses the owning object's world matrix (four corners); suitable for orthographic 2D scenes.
 */
class BoxCollider2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::BoxCollider2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit BoxCollider2DComponent(Vector2 localHalfExtents = {0.5F, 0.5F}, Vector2 localOffset = Vector2::Zero) noexcept
            : halfExtents(localHalfExtents), offset(localOffset) {}

    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }
    [[nodiscard]] const Vector2& GetOffset() const noexcept { return offset; }

    void SetHalfExtents(const Vector2& h) noexcept { halfExtents = h; }
    void SetOffset(const Vector2& o) noexcept { offset = o; }

    /** Layer bitmask (usually one bit). Default = layer 0. */
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    void SetCategoryBits(std::uint16_t bits) noexcept { categoryBits = bits; }

    /** Layers this collider interacts with (bitmask). Default = all 16 layers. */
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    void SetMaskBits(std::uint16_t bits) noexcept { maskBits = bits; }

    /** If true, no blocking — overlap only (see <c>SignalId::Physics2DTriggerOverlap</c> on the trigger object). */
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    void SetIsTrigger(bool isTrigger) noexcept { this->isTrigger = isTrigger; }

private:
    Vector2 halfExtents{0.5F, 0.5F};
    Vector2 offset{Vector2::Zero};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
