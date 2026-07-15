#include "spark/ecs/components/GuiCanvasComponent.hpp"

#include "spark/core/Array.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/Widget.hpp"

#include <GLFW/glfw3.h>

namespace Spark {

namespace {

[[nodiscard]] Gui::Widget* FindFocusableFromHit(Gui::Widget* hit) noexcept {
    for (Gui::Widget* w = hit; w != nullptr; w = w->GetParent()) {
        if (w->WantsKeyboardFocus()) {
            return w;
        }
    }
    return nullptr;
}

void CollectTabOrder(Gui::Widget* w, Array<Gui::Widget*>& out) {
    if (w == nullptr || !w->IsVisible() || !w->IsEnabled()) {
        return;
    }
    if (w->WantsKeyboardFocus()) {
        out.PushBack(w);
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        CollectTabOrder(ch[i].Get(), out);
    }
}

}  // namespace

void GuiCanvasComponent::ClearTransientPointerState() noexcept {
    hotWidget = nullptr;
}

void GuiCanvasComponent::StepPointer(const Gui::GuiFrameInput& in) {
    lastFrameInput = in;
    if (!canvasEnabled || !root) {
        hotWidget = nullptr;
        activePress = nullptr;
        return;
    }
    if (!root->IsVisible()) {
        hotWidget = nullptr;
        return;
    }

    if (frameHotWidget_ != nullptr) {
        hotWidget = frameHotWidget_;
        frameHotWidget_ = nullptr;
    } else {
        hotWidget = root->FindDeepestHover(in.mouseX, in.mouseY);
    }

    if (in.leftPressedThisFrame) {
        if (hotWidget != nullptr) {
            activePress = hotWidget;
            hotWidget->NotifyPointerDown(in, *this);
            if (Gui::Widget* focusTarget = FindFocusableFromHit(hotWidget)) {
                focusWidget = focusTarget;
            }
        } else if (activePress == nullptr) {
            focusWidget = nullptr;
        }
    }

    if (in.leftReleasedThisFrame) {
        if (activePress != nullptr) {
            activePress->NotifyPointerUp(in, *this);
            /** Use subtree hit-test so a list row Button still receives NotifyClick when a sibling sits above it in
             *  the full canvas (hotWidget may point at the overlapping widget). */
            const bool stillOver =
                    activePress->FindDeepestHover(in.mouseX, in.mouseY) != nullptr;
            if (stillOver) {
                activePress->NotifyClick(in, *this);
            }
        }
        activePress = nullptr;
    }

    if (in.leftDown && activePress != nullptr) {
        activePress->NotifyPointerDrag(in, *this);
    }
}

void GuiCanvasComponent::ProcessKeyFocus(IInput& input) {
    if (!canvasEnabled || !root || !root->IsVisible()) {
        return;
    }
    const bool tab = input.IsKeyPressedThisFrame(GLFW_KEY_TAB);
    const bool shiftTab =
            tab && (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT));
    if (tab) {
        Array<Gui::Widget*> order;
        CollectTabOrder(root.Get(), order);
        if (order.IsEmpty()) {
            return;
        }
        int cur = -1;
        for (std::size_t i = 0; i < order.GetSize(); ++i) {
            if (order[i] == focusWidget) {
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
        focusWidget = order[static_cast<std::size_t>(next)];
        return;
    }
    if (focusWidget != nullptr) {
        focusWidget->ProcessKeyInput(input);
    }
}

void GuiCanvasComponent::Paint(Gui::GuiPaintContext& ctx) const {
    if (!canvasEnabled || !root || !root->IsVisible()) {
        return;
    }
    ctx.ClearClipStack();
    ctx.SetTheme(&guiTheme);
    ctx.SetLayoutMetrics(&layoutMetrics);
    ctx.SetInteraction(hotWidget, activePress, focusWidget);
    root->Paint(ctx);
}

}  // namespace Spark
