#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

class IInput;
class UiCanvasComponent;

namespace Ui {

class IUiRenderer;

/**
 * Base contract for every control and container in a retained UI tree.
 * Backends implement concrete elements (<c>SparkButton</c>, <c>ImguiButton</c>, …).
 */
class IUiElement {
public:
    virtual ~IUiElement() = default;

    [[nodiscard]] virtual UiElementId GetId() const noexcept = 0;
    [[nodiscard]] virtual Rect GetBounds() const noexcept = 0;

    virtual void Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) = 0;
    virtual void Arrange(const Rect& finalBounds) = 0;
    virtual void Paint(IUiRenderer& renderer) = 0;

    [[nodiscard]] virtual IUiElement* HitTest(const float x, const float y) = 0;
    [[nodiscard]] virtual const IUiElement* HitTest(const float x, const float y) const = 0;

    virtual void OnPointerDown(const UiFrameInput& input, UiCanvasComponent& canvas) {}
    virtual void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) {}
    virtual void OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& canvas) {}
    virtual void OnScroll(const float deltaX, const float deltaY) {}
    virtual bool OnKey(const int key, const bool pressed) { return false; }
    virtual void ProcessKeyInput(IInput& input) {}

    [[nodiscard]] virtual IUiElement* GetParent() noexcept { return nullptr; }
    [[nodiscard]] virtual const IUiElement* GetParent() const noexcept { return nullptr; }
    [[nodiscard]] virtual bool WantsScrollInput() const { return false; }
    [[nodiscard]] virtual bool WantsKeyboardFocus() const { return false; }
    virtual void OnFocusGained() {}
    virtual void OnFocusLost() {}

    [[nodiscard]] virtual bool IsVisible() const noexcept = 0;
    [[nodiscard]] virtual bool IsEnabled() const noexcept = 0;
    [[nodiscard]] virtual bool WantsHitTest() const noexcept = 0;

    virtual void AddChild(UniquePtr<IUiElement> child) = 0;
    [[nodiscard]] virtual const Array<UniquePtr<IUiElement>>& GetChildren() const noexcept = 0;
};

}  // namespace Ui
}  // namespace Spark
