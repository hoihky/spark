#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <cstdint>

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Checkerboard tile preview. When <c>textureLayer >= 0</c>, draws a textured quad via <c>EmitSprite</c> (expects
 * <c>SceneRenderParams::viewProjection</c> in pixel HUD space matching these bounds).
 */
class TileSwatch final : public Widget {
public:
    void SetCheckerSize(float px) noexcept { checkerPx = px > 2.0F ? px : 8.0F; }
    void SetTextureLayer(std::int32_t layer) noexcept { textureLayer = layer; }
    void SetCaption(Utf8String t) { caption = Spark::MoveTemp(t); }

    void Paint(GuiPaintContext& ctx) const override;

private:
    float checkerPx = 8.0F;
    std::int32_t textureLayer = -1;
    Utf8String caption{};
};

}  // namespace Spark::Gui
