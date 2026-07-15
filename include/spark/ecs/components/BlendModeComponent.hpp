#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/render/SceneBlendMode.hpp"

namespace Spark {

/**
 * Optional compositing mode for 2D drawables on the same <c>GameObject</c>.
 * Consumed when building <c>SceneSpriteDraw</c> (pair with <c>SpriteComponent</c> or <c>TilemapComponent</c>).
 * Screen UI sets <c>SceneBlendMode</c> directly on <c>ScreenRectDraw</c> / via <c>GuiPaintContext</c>.
 */
class BlendModeComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::BlendMode;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit BlendModeComponent(SceneBlendMode modeIn = kSceneBlendModeDefault) noexcept : mode(modeIn) {}

    [[nodiscard]] SceneBlendMode GetMode() const noexcept { return mode; }
    void SetMode(SceneBlendMode m) noexcept { mode = m; }

private:
    SceneBlendMode mode = kSceneBlendModeDefault;
};

}  // namespace Spark
