#pragma once

#include "spark/core/TypeTraits.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiElement.hpp"
#include "spark/ui/core/UiTheme.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

class IInput;

/**
 * ECS hook for a retained-mode UI tree built with <c>Spark::Ui::IUiElement</c>.
 * Input is routed by <c>UiInputRouter</c>; paint goes through <c>SparkUiRenderer</c> into
 * <c>SceneRenderParams</c> for the Vulkan screen UI pass.
 */
class UiCanvasComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::UiCanvas;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetSortOrder(int order) noexcept { sortOrder = order; }
    [[nodiscard]] int GetSortOrder() const noexcept { return sortOrder; }

    void SetCanvasEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IsCanvasEnabled() const noexcept { return canvasEnabled; }

    template<typename T>
        requires DerivedFrom<T, Ui::IUiElement>
    void SetRoot(UniquePtr<T> element) {
        if (element) {
            root = UniquePtr<Ui::IUiElement>(static_cast<Ui::IUiElement*>(element.Release()));
        } else {
            root.Reset();
        }
    }

    [[nodiscard]] Ui::IUiElement* GetRoot() noexcept { return root.Get(); }
    [[nodiscard]] const Ui::IUiElement* GetRoot() const noexcept { return root.Get(); }

    void ClearTransientPointerState() noexcept;
    void ClearKeyboardFocus() noexcept;
    void ApplyFocus(Ui::IUiElement* nextFocus) noexcept;

    /** Called by <c>UiInputRouter</c> with the topmost hit under the cursor (may be nullptr). */
    void StepPointer(const Ui::UiFrameInput& frameInput, Ui::IUiElement* hitElement);
    void ProcessKeyFocus(IInput& input);

    void Paint(Ui::IUiRenderer& renderer) const;

    void SetTheme(Ui::UiTheme theme) noexcept { uiTheme = theme; }
    [[nodiscard]] const Ui::UiTheme& GetTheme() const noexcept { return uiTheme; }

    void SetModalInputCapture(bool capture) noexcept { modalInputCapture = capture; }
    [[nodiscard]] bool GetModalInputCapture() const noexcept { return modalInputCapture; }

    [[nodiscard]] Ui::IUiElement* GetHotElement() const noexcept { return hotElement; }
    [[nodiscard]] Ui::IUiElement* GetActivePressElement() const noexcept { return activePress; }
    [[nodiscard]] Ui::IUiElement* GetFocusElement() const noexcept { return focusElement; }

    [[nodiscard]] const Ui::UiFrameInput& GetLastFrameInput() const noexcept { return lastFrameInput; }

private:
    int sortOrder = 0;
    bool canvasEnabled = true;
    UniquePtr<Ui::IUiElement> root{};
    Ui::UiTheme uiTheme{Ui::UiTheme::ClassicMint()};

    Ui::IUiElement* hotElement = nullptr;
    Ui::IUiElement* activePress = nullptr;
    Ui::IUiElement* focusElement = nullptr;
    Ui::UiFrameInput lastFrameInput{};
    bool modalInputCapture = false;
};

}  // namespace Spark
