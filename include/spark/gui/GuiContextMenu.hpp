#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/gui/GuiTypes.hpp"

#include <functional>

namespace Spark {

class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

/** Global modal context menu (overlay layer). Opened on right-click via <c>ProcessGuiCanvasesInput</c>. */
class GuiContextMenu {
public:
    void Open(float x, float y, Array<Utf8String> itemLabels, std::function<void(int index)> onPick);
    void Close() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return open; }

    /** Returns true if the menu consumed the click. */
    bool HandlePointer(const GuiFrameInput& in, GuiCanvasComponent& canvas);
    void Paint(GuiPaintContext& ctx) const;

private:
    bool open = false;
    float anchorX = 0.0F;
    float anchorY = 0.0F;
    Array<Utf8String> items{};
    std::function<void(int index)> onPick{};
    int hoverIndex = -1;
    Rect panelRect{};
    float rowHeight = 32.0F;
};

[[nodiscard]] GuiContextMenu& GetGuiContextMenu() noexcept;

}  // namespace Gui
}  // namespace Spark
