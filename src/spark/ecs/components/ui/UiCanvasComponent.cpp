#include "spark/ecs/components/ui/UiCanvasComponent.hpp"

#include <GLFW/glfw3.h>

#include "spark/core/Array.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/ui/core/IUiRenderer.hpp"

namespace Spark {

namespace {

[[nodiscard]] Ui::IUiElement* FindFocusableFromHit(Ui::IUiElement* hit) noexcept {
    for (Ui::IUiElement* element = hit; element != nullptr; element = element->GetParent()) {
        if (element->WantsKeyboardFocus()) {
            return element;
        }
    }
    return nullptr;
}

void CollectTabOrder(Ui::IUiElement* element, Array<Ui::IUiElement*>& out) {
    if (element == nullptr || !element->IsVisible() || !element->IsEnabled()) {
        return;
    }
    if (element->WantsKeyboardFocus()) {
        out.PushBack(element);
    }
    const Array<UniquePtr<Ui::IUiElement>>& children = element->GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        CollectTabOrder(children[i].Get(), out);
    }
}

}  // namespace

void UiCanvasComponent::SetCanvasEnabled(const bool enabled) noexcept {
    if (canvasEnabled && !enabled) {
        hotElement = nullptr;
        activePress = nullptr;
        ApplyFocus(nullptr);
    }
    canvasEnabled = enabled;
}

void UiCanvasComponent::ClearTransientPointerState() noexcept {
    hotElement = nullptr;
}

void UiCanvasComponent::ClearKeyboardFocus() noexcept {
    ApplyFocus(nullptr);
}

void UiCanvasComponent::ApplyFocus(Ui::IUiElement* nextFocus) noexcept {
    if (focusElement == nextFocus) {
        return;
    }
    if (focusElement != nullptr) {
        focusElement->OnFocusLost();
    }
    focusElement = nextFocus;
    if (focusElement != nullptr) {
        focusElement->OnFocusGained();
    }
}

void UiCanvasComponent::StepPointer(const Ui::UiFrameInput& in, Ui::IUiElement* hitElement) {
    lastFrameInput = in;
    if (!canvasEnabled || root == nullptr) {
        hotElement = nullptr;
        activePress = nullptr;
        return;
    }
    if (!root->IsVisible()) {
        hotElement = nullptr;
        return;
    }

    hotElement = hitElement;

    if (in.leftPressedThisFrame) {
        if (hotElement != nullptr) {
            activePress = hotElement;
            hotElement->OnPointerDown(in, *this);
            if (Ui::IUiElement* focusTarget = FindFocusableFromHit(hotElement)) {
                ApplyFocus(focusTarget);
            }
        } else if (activePress == nullptr) {
            ApplyFocus(nullptr);
        }
    }

    if (in.leftReleasedThisFrame) {
        if (activePress != nullptr) {
            activePress->OnPointerUp(in, *this);
        }
        activePress = nullptr;
    }

    if (in.leftDown && activePress != nullptr) {
        activePress->OnPointerDrag(in, *this);
    }
}

void UiCanvasComponent::ProcessKeyFocus(IInput& input) {
    if (!canvasEnabled || root == nullptr || !root->IsVisible()) {
        return;
    }
    const bool tab = input.IsKeyPressedThisFrame(GLFW_KEY_TAB);
    const bool shiftTab =
            tab && (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT));
    if (tab) {
        Array<Ui::IUiElement*> order;
        CollectTabOrder(root.Get(), order);
        if (order.IsEmpty()) {
            return;
        }
        int cur = -1;
        for (std::size_t i = 0; i < order.GetSize(); ++i) {
            if (order[i] == focusElement) {
                cur = static_cast<int>(i);
                break;
            }
        }
        int next = 0;
        if (cur >= 0) {
            if (shiftTab) {
                next = cur - 1;
                if (next < 0) {
                    next = static_cast<int>(order.GetSize()) - 1;
                }
            } else {
                next = cur + 1;
                if (next >= static_cast<int>(order.GetSize())) {
                    next = 0;
                }
            }
        }
        ApplyFocus(order[static_cast<std::size_t>(next)]);
        return;
    }
    if (focusElement != nullptr) {
        focusElement->ProcessKeyInput(input);
    }
}

void UiCanvasComponent::Paint(Ui::IUiRenderer& renderer) const {
    if (!canvasEnabled || root == nullptr || !root->IsVisible()) {
        return;
    }
    renderer.SetTheme(&uiTheme);
    renderer.SetInteraction(hotElement, activePress, focusElement);
    root->Paint(renderer);
}

}  // namespace Spark
