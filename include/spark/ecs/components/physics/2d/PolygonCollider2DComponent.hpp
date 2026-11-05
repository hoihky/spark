#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/** Convex polygon collider in local XY (static bodies only; max 16 vertices). */
class PolygonCollider2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PolygonCollider2D;
    static constexpr std::uint32_t MaxVertices = 16;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    PolygonCollider2DComponent() = default;

    [[nodiscard]] std::uint32_t GetVertexCount() const noexcept {
        return static_cast<std::uint32_t>(vertices.GetSize());
    }
    [[nodiscard]] const Array<Vector2>& GetVertices() const noexcept { return vertices; }
    Array<Vector2>& GetVertices() noexcept { return vertices; }

    void SetVertices(const Array<Vector2>& verts);
    void ClearVertices() noexcept { vertices.Clear(); }

    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    void SetCategoryBits(std::uint16_t bits) noexcept { categoryBits = bits; }
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    void SetMaskBits(std::uint16_t bits) noexcept { maskBits = bits; }
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    void SetIsTrigger(bool value) noexcept { isTrigger = value; }

private:
    Array<Vector2> vertices{};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
