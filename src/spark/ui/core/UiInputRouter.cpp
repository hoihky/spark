#include "spark/ui/core/UiInputRouter.hpp"

#include <GLFW/glfw3.h>

#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"

namespace Spark::Ui {

namespace {

void CollectUiCanvases(const GameWorld& world, Array<UiCanvasComponent*>& out) {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (UiCanvasComponent* canvas = object->GetComponent<UiCanvasComponent>()) {
            if (canvas->IsCanvasEnabled()) {
                out.PushBack(canvas);
            }
        }
    });
}

void SortCanvasesByOrder(Array<UiCanvasComponent*>& list) {
    for (std::size_t i = 1; i < list.GetSize(); ++i) {
        UiCanvasComponent* key = list[i];
        std::size_t j = i;
        while (j > 0 && list[j - 1U]->GetSortOrder() > key->GetSortOrder()) {
            list[j] = list[j - 1U];
            --j;
        }
        list[j] = key;
    }
}

[[nodiscard]] UiCanvasComponent* FindModalCanvas(const Array<UiCanvasComponent*>& canvases) {
    for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
        UiCanvasComponent* canvas = canvases[ri - 1U];
        if (canvas != nullptr && canvas->IsCanvasEnabled() && canvas->GetModalInputCapture()) {
            return canvas;
        }
    }
    return nullptr;
}

[[nodiscard]] bool ShouldProcessCanvas(const UiCanvasComponent* canvas, const UiCanvasComponent* modalCanvas) {
    if (canvas == nullptr || !canvas->IsCanvasEnabled()) {
        return false;
    }
    if (modalCanvas != nullptr && canvas != modalCanvas) {
        return false;
    }
    return true;
}

[[nodiscard]] IUiElement* FindScrollableAncestor(IUiElement* hit) noexcept {
    for (IUiElement* element = hit; element != nullptr; element = element->GetParent()) {
        if (element->WantsScrollInput()) {
            return element;
        }
    }
    return nullptr;
}

[[nodiscard]] IUiElement* FindTopmostScrollableUnderPoint(IUiElement* element, const float x, const float y) {
    if (element == nullptr || !element->IsVisible() || !element->IsEnabled()) {
        return nullptr;
    }
    const Array<UniquePtr<IUiElement>>& children = element->GetChildren();
    for (std::size_t ci = children.GetSize(); ci > 0; --ci) {
        if (IUiElement* hit = FindTopmostScrollableUnderPoint(children[ci - 1U].Get(), x, y)) {
            return hit;
        }
    }
    if (element->WantsScrollInput() && element->GetBounds().Contains(x, y)) {
        return element;
    }
    return nullptr;
}

void ArrangeCanvasRoot(UiCanvasComponent* canvas, const Rect& viewport) {
    if (canvas == nullptr) {
        return;
    }
    IUiElement* root = canvas->GetRoot();
    if (root == nullptr) {
        return;
    }
    UiMeasureConstraints constraints{};
    constraints.maxWidth = viewport.width;
    constraints.maxHeight = viewport.height;
    UiSize desired{};
    root->Measure(constraints, desired);
    (void)desired;
    root->Arrange(viewport);
}

}  // namespace

UiFrameInput UiInputRouter::BuildFrameInput(IInput& input, const int framebufferWidth, const int framebufferHeight) {
    UiFrameInput frame{};
    input.GetCursorFramebufferPixels(frame.mouseX, frame.mouseY, framebufferWidth, framebufferHeight);
    frame.leftDown = input.IsMouseButtonDown(0);
    frame.leftPressedThisFrame = input.IsMouseButtonPressedThisFrame(0);
    frame.leftReleasedThisFrame = input.IsMouseButtonReleasedThisFrame(0);
    frame.rightDown = input.IsMouseButtonDown(1);
    frame.rightPressedThisFrame = input.IsMouseButtonPressedThisFrame(1);
    frame.ctrlDown = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
    frame.shiftDown = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
    frame.altDown = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);
    frame.scrollDeltaY = input.GetScrollDeltaY();
    return frame;
}

void UiInputRouter::Process(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    Array<UiCanvasComponent*> canvases;
    CollectUiCanvases(world, canvases);
    SortCanvasesByOrder(canvases);

    for (std::size_t i = 0; i < canvases.GetSize(); ++i) {
        if (canvases[i] != nullptr) {
            canvases[i]->ClearTransientPointerState();
        }
    }
    (void)contentScaleX;
    (void)contentScaleY;

    const UiFrameInput frameInput = BuildFrameInput(input, framebufferWidth, framebufferHeight);
    const Rect viewport{
            0.0F,
            0.0F,
            static_cast<float>(framebufferWidth > 0 ? framebufferWidth : 1),
            static_cast<float>(framebufferHeight > 0 ? framebufferHeight : 1)};

    UiCanvasComponent* modalCanvas = FindModalCanvas(canvases);

    hotElement = nullptr;
    activeElement = nullptr;
    focusElement = nullptr;
    activeCanvas = nullptr;

    IUiElement* pointerHit = nullptr;
    UiCanvasComponent* pointerOwner = nullptr;

    for (std::size_t i = 0; i < canvases.GetSize(); ++i) {
        UiCanvasComponent* canvas = canvases[i];
        if (!ShouldProcessCanvas(canvas, modalCanvas)) {
            continue;
        }
        ArrangeCanvasRoot(canvas, viewport);
    }

    for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
        UiCanvasComponent* canvas = canvases[ri - 1U];
        if (!ShouldProcessCanvas(canvas, modalCanvas)) {
            continue;
        }
        if (canvas->GetRoot() == nullptr) {
            continue;
        }
        if (IUiElement* hit = canvas->GetRoot()->HitTest(frameInput.mouseX, frameInput.mouseY)) {
            pointerHit = hit;
            pointerOwner = canvas;
            hotElement = hit;
            break;
        }
    }

    UiCanvasComponent* activeCaptureCanvas = nullptr;
    for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
        UiCanvasComponent* canvas = canvases[ri - 1U];
        if (canvas != nullptr && canvas->GetActivePressElement() != nullptr) {
            activeCaptureCanvas = canvas;
            break;
        }
    }

    UiCanvasComponent* stepCanvas = pointerOwner != nullptr ? pointerOwner : activeCaptureCanvas;
    if (modalCanvas != nullptr) {
        stepCanvas = modalCanvas;
        if (pointerOwner != modalCanvas) {
            pointerHit = nullptr;
            if (modalCanvas->GetRoot() != nullptr) {
                pointerHit = modalCanvas->GetRoot()->HitTest(frameInput.mouseX, frameInput.mouseY);
                hotElement = pointerHit;
            }
        }
    }

    bool scrollWheelConsumed = false;
    if (stepCanvas != nullptr && frameInput.scrollDeltaY != 0.0F) {
        IUiElement* scrollTarget = nullptr;
        if (pointerOwner != nullptr) {
            scrollTarget = FindScrollableAncestor(hotElement);
            if (scrollTarget == nullptr && pointerOwner->GetRoot() != nullptr) {
                scrollTarget = FindTopmostScrollableUnderPoint(
                        pointerOwner->GetRoot(), frameInput.mouseX, frameInput.mouseY);
            }
        } else if (activeCaptureCanvas != nullptr && activeCaptureCanvas->GetRoot() != nullptr) {
            scrollTarget = FindTopmostScrollableUnderPoint(
                    activeCaptureCanvas->GetRoot(), frameInput.mouseX, frameInput.mouseY);
        }
        if (scrollTarget != nullptr) {
            scrollTarget->OnScroll(0.0F, frameInput.scrollDeltaY);
            scrollWheelConsumed = true;
            ArrangeCanvasRoot(stepCanvas, viewport);
        }
    }

    if (stepCanvas != nullptr) {
        stepCanvas->StepPointer(frameInput, pointerHit);
        if (frameInput.leftPressedThisFrame) {
            for (std::size_t i = 0; i < canvases.GetSize(); ++i) {
                UiCanvasComponent* canvas = canvases[i];
                if (canvas == nullptr || canvas == stepCanvas) {
                    continue;
                }
                canvas->ClearTransientPointerState();
                canvas->ClearKeyboardFocus();
            }
        }
        ArrangeCanvasRoot(stepCanvas, viewport);
        focusElement = stepCanvas->GetFocusElement();
        activeElement = stepCanvas->GetActivePressElement();
        activeCanvas = stepCanvas;
    }

    for (std::size_t i = 0; i < canvases.GetSize(); ++i) {
        UiCanvasComponent* canvas = canvases[i];
        if (canvas == nullptr || canvas == stepCanvas) {
            continue;
        }
        canvas->StepPointer(frameInput, nullptr);
    }

  UiCanvasComponent* keyCanvas = modalCanvas;
    if (keyCanvas == nullptr) {
        for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
            UiCanvasComponent* canvas = canvases[ri - 1U];
            if (canvas != nullptr && canvas->GetFocusElement() != nullptr) {
                keyCanvas = canvas;
                break;
            }
        }
    }
    if (keyCanvas == nullptr && pointerOwner != nullptr && pointerOwner->GetFocusElement() != nullptr) {
        keyCanvas = pointerOwner;
    }
    if (keyCanvas != nullptr) {
        keyCanvas->ProcessKeyFocus(input);
        focusElement = keyCanvas->GetFocusElement();
    } else if (!canvases.IsEmpty() && input.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
        UiCanvasComponent* target = pointerOwner;
        if (!ShouldProcessCanvas(target, modalCanvas)) {
            target = nullptr;
        }
        if (target == nullptr) {
            for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
                UiCanvasComponent* canvas = canvases[ri - 1U];
                if (ShouldProcessCanvas(canvas, modalCanvas) && canvas->GetRoot() != nullptr) {
                    target = canvas;
                    break;
                }
            }
        }
        if (target != nullptr) {
            target->ProcessKeyFocus(input);
            focusElement = target->GetFocusElement();
        }
    }

    captureCanvas = nullptr;
    for (std::size_t ri = canvases.GetSize(); ri > 0; --ri) {
        UiCanvasComponent* canvas = canvases[ri - 1U];
        if (canvas != nullptr && canvas->GetActivePressElement() != nullptr) {
            captureCanvas = canvas;
            break;
        }
    }

    UiPointerState pointerState{};
    pointerState.mouseX = frameInput.mouseX;
    pointerState.mouseY = frameInput.mouseY;
    pointerState.pointerOverUi = hotElement != nullptr;

    GetUiContextMenu().SetViewportBounds(viewport.width, viewport.height);

    if (GetUiContextMenu().IsOpen()) {
        (void)GetUiContextMenu().HandlePointer(frameInput);
        pointerState.pointerOverUi = true;
        pointerState.consumesGamePointer = true;
    } else {
        pointerState.consumesGamePointer =
                pointerState.pointerOverUi || (captureCanvas != nullptr && frameInput.leftDown);
    }
    pointerState.scrollWheelConsumed = scrollWheelConsumed;
    SetUiPointerState(pointerState);
}

}  // namespace Spark::Ui
