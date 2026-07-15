#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/render/SpriteLighting2D.hpp"

namespace Spark {

/**
 * Optional per-sprite 2D lighting / shading (see SpriteLighting2DMode). Consumed when building SceneSpriteDraw;
 * fragment work happens in sprite.frag. Pair with SpriteComponent on the same GameObject.
 */
class SpriteLighting2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpriteLighting2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SpriteLighting2DComponent(
            SpriteLighting2DMode modeIn = SpriteLighting2DMode::None,
            Vector4 param0In = {1.0F, 1.0F, 1.0F, 1.0F},
            Vector4 param1In = {1.0F, 0.0F, 0.0F, 0.0F}) noexcept
            : mode(modeIn), param0(param0In), param1(param1In) {}

    [[nodiscard]] SpriteLighting2DMode GetMode() const noexcept { return mode; }
    [[nodiscard]] const Vector4& GetParam0() const noexcept { return param0; }
    [[nodiscard]] const Vector4& GetParam1() const noexcept { return param1; }

    void SetMode(SpriteLighting2DMode m) noexcept { mode = m; }
    void SetParam0(const Vector4& v) noexcept { param0 = v; }
    void SetParam1(const Vector4& v) noexcept { param1 = v; }

private:
    SpriteLighting2DMode mode = SpriteLighting2DMode::None;
    Vector4 param0{1.0F, 1.0F, 1.0F, 1.0F};
    Vector4 param1{1.0F, 0.0F, 0.0F, 0.0F};
};

}  // namespace Spark
