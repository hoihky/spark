#pragma once

#include "spark/core/Array.hpp"
#include "spark/ui/core/UiPointerState.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

class GameWorld;
class IInput;
class UiCanvasComponent;

namespace Ui {

class IUiElement;

/**
 * Single input pass for all <c>UiCanvasComponent</c> trees: measure, arrange, hit-test, focus, capture.
 */
class UiInputRouter {
public:
    void Process(
            GameWorld& world,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX,
            float contentScaleY);

    [[nodiscard]] IUiElement* GetHotElement() const noexcept { return hotElement; }
    [[nodiscard]] IUiElement* GetActiveElement() const noexcept { return activeElement; }
    [[nodiscard]] IUiElement* GetFocusElement() const noexcept { return focusElement; }

private:
    static UiFrameInput BuildFrameInput(IInput& input, int framebufferWidth, int framebufferHeight);

    IUiElement* hotElement = nullptr;
    IUiElement* activeElement = nullptr;
    IUiElement* focusElement = nullptr;
    UiCanvasComponent* activeCanvas = nullptr;
    UiCanvasComponent* captureCanvas = nullptr;
};

}  // namespace Ui
}  // namespace Spark
