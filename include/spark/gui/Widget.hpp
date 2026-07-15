#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/TypeTraits.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class GuiCanvasComponent;
class IInput;

namespace Gui {

class GuiPaintContext;

/**
 * Base of the retained-mode GUI tree. Subclass for custom controls; compose built-ins from GuiControls.hpp.
 * Bounds are absolute framebuffer pixels updated during Arrange.
 */
class Widget {
public:
    Widget() = default;
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    [[nodiscard]] bool IsVisible() const noexcept { return visible; }
    void SetVisible(bool v) noexcept { visible = v; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(bool e) noexcept { enabled = e; }

    [[nodiscard]] bool WantsHitTest() const noexcept { return hitTest; }
    void SetHitTest(bool h) noexcept { hitTest = h; }

    void SetTooltip(Utf8String text) { tooltip = Spark::MoveTemp(text); }
    [[nodiscard]] const Utf8String& GetTooltip() const noexcept { return tooltip; }

    [[nodiscard]] const Rect& GetBounds() const noexcept { return bounds; }
    [[nodiscard]] Widget* GetParent() noexcept { return parent; }
    [[nodiscard]] const Widget* GetParent() const noexcept { return parent; }

    /** When true, clicking this widget (or a non-focusable descendant) may claim keyboard focus (see GuiCanvasComponent). */
    [[nodiscard]] virtual bool WantsKeyboardFocus() const { return false; }

    template<typename T>
        requires DerivedFrom<T, Widget>
    void AddChild(UniquePtr<T> child) {
        if (child) {
            Widget* w = static_cast<Widget*>(child.Release());
            if (w->parent != nullptr) {
                w->parent->RemoveChild(w);
            }
            w->parent = this;
            children.PushBack(UniquePtr<Widget>(w));
        }
    }

    [[nodiscard]] const Array<UniquePtr<Widget>>& GetChildren() const noexcept { return children; }

    /** Removes one direct child; no-op if not found. */
    void RemoveChild(Widget* child);

    /** Removes all child widgets (used by tab hosts and similar). */
    void ClearChildren();

    /** Assign final screen-space bounds (call on root with full viewport rect, then children). */
    virtual void Arrange(const Rect& r);

    /** Deepest widget under (x,y) in screen space; children painted later are tested first. */
    [[nodiscard]] virtual Widget* FindDeepestHover(float x, float y);

    virtual void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas);
    virtual void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas);
    /** Called each frame while the left button is held after <c>NotifyPointerDown</c> on this widget. */
    virtual void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas);
    virtual void NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas);

    virtual void Paint(GuiPaintContext& ctx) const;

    /** When this widget or a descendant has text focus; default no-op. */
    virtual void ProcessKeyInput(IInput& input);

protected:
    void PaintChildren(GuiPaintContext& ctx) const;

    Rect bounds{};
    Widget* parent = nullptr;
    bool visible = true;
    bool enabled = true;
    bool hitTest = true;
    Utf8String tooltip{};
    Array<UniquePtr<Widget>> children{};
};

}  // namespace Gui
}  // namespace Spark
