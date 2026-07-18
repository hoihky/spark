#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark {

/**
 * Unlit textured quad (unit square in XY, z=0) submitted to the alpha sprite pass.
 * Transform on the owning object scales/orients the quad; tint multiplies texture; uvRect selects an atlas region.
 *
 * <c>sortOrder</c> is the drawable-local draw key (lower draws first). For named layers and sorting groups,
 * see <c>RenderLayerComponent</c> and <c>SortingGroupComponent</c> (resolved at scene submit).
 * For top-down ARPG occlusion within a layer, enable <c>SceneSpriteSortMode::SortOrderThenWorldY</c> on scene submit; tie-break then uses world translation Y from the
 * sprite’s world matrix (after <c>GetWorldMatrix</c>).
 */
class SpriteComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Sprite;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SpriteComponent() = default;
    SpriteComponent(
            SharedPtr<Texture2D> inTexture,
            const Vector4& inTint,
            const Vector4& inUvRect,
            std::int32_t inSortOrder) noexcept;

    [[nodiscard]] const SharedPtr<Texture2D>& GetTexture() const noexcept { return texture; }
    [[nodiscard]] const Vector4& GetTint() const noexcept { return tint; }
    [[nodiscard]] const Vector4& GetUvRect() const noexcept { return uvRect; }
    [[nodiscard]] std::int32_t GetSortOrder() const noexcept { return sortOrder; }

    void SetTexture(SharedPtr<Texture2D> t) noexcept { texture = MoveTemp(t); }
    void SetTint(const Vector4& c) noexcept { tint = c; }
    void SetUvRect(const Vector4& uv) noexcept { uvRect = uv; }
    void SetSortOrder(std::int32_t o) noexcept { sortOrder = o; }

private:
    SharedPtr<Texture2D> texture{};
    Vector4 tint{1.0F, 1.0F, 1.0F, 1.0F};
    Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};
    std::int32_t sortOrder = 0;
};

}  // namespace Spark
