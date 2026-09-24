#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Function.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

namespace Ui {

class UiPaintContext;

/** Global modal context menu (overlay layer). */
class UiContextMenu {
public:
    /** Opens at framebuffer pixel coordinates (top-left origin, same as <c>GetCursorFramebufferPixels</c>). */
    void Open(float x, float y, Array<Utf8String> itemLabels, Function<void(int index)> onPick);
    void Close() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return open; }

    bool HandlePointer(const UiFrameInput& in);
    void SetViewportBounds(float width, float height) noexcept;
    void Paint(UiPaintContext& ctx) const;
    void PaintImGui();

private:
    bool open = false;
    float anchorX = 0.0F;
    float anchorY = 0.0F;
    Array<Utf8String> items{};
    Function<void(int index)> onPick{};
    int hoverIndex = -1;
    Rect panelRect{};
    float rowHeight = 32.0F;
    float viewportWidth = 0.0F;
    float viewportHeight = 0.0F;
    bool imguiPopupRequested = false;
};

[[nodiscard]] UiContextMenu& GetUiContextMenu() noexcept;

}  // namespace Ui
}  // namespace Spark
