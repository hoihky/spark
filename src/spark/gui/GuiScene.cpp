#include "spark/gui/GuiScene.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/gui/GuiContextMenu.hpp"
#include "spark/gui/controls/Dialog.hpp"
#include "spark/gui/GuiInputState.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiWidgetPopups.hpp"
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/imgui/ImGuiLayerRegistry.hpp"
#include "spark/gui/controls/Dropdown.hpp"
#include "spark/gui/controls/List.hpp"
#include "spark/gui/controls/MenuBar.hpp"
#include "spark/gui/controls/MultiSelectList.hpp"
#include "spark/gui/controls/ScrollPanel.hpp"
#include "spark/gui/controls/TreeView.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/text/Font.hpp"

#include <GLFW/glfw3.h>

namespace Spark {

namespace {

void CollectCanvases(const GameWorld& world, Array<GuiCanvasComponent*>& out) {
    out.Clear();
    world.ForEachActiveGameObject([&out](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (GuiCanvasComponent* g = o->GetComponent<GuiCanvasComponent>()) {
            out.PushBack(g);
        }
    });
}

Gui::Widget* FindTopmostScrollableUnderPoint(Gui::Widget* w, const float x, const float y) {
    if (w == nullptr || !w->IsVisible() || !w->IsEnabled()) {
        return nullptr;
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = ch.GetSize(); i > 0; --i) {
        if (Gui::Widget* hit = FindTopmostScrollableUnderPoint(ch[i - 1U].Get(), x, y)) {
            return hit;
        }
    }
    if (auto* list = dynamic_cast<Gui::List*>(w)) {
        if (list->IsVerticalScrollingEnabled() && list->GetBounds().Contains(x, y)) {
            return list;
        }
    } else if (auto* mlist = dynamic_cast<Gui::MultiSelectList*>(w)) {
        if (mlist->IsVerticalScrollingEnabled() && mlist->GetBounds().Contains(x, y)) {
            return mlist;
        }
    } else if (auto* tree = dynamic_cast<Gui::TreeView*>(w)) {
        if (tree->GetBounds().Contains(x, y)) {
            return tree;
        }
    } else if (auto* panel = dynamic_cast<Gui::ScrollPanel*>(w)) {
        if (panel->GetBounds().Contains(x, y)) {
            return panel;
        }
    }
    return nullptr;
}

Gui::Widget* FindScrollableAncestor(Gui::Widget* hit) noexcept {
    for (Gui::Widget* w = hit; w != nullptr; w = w->GetParent()) {
        if (auto* list = dynamic_cast<Gui::List*>(w)) {
            if (list->IsVerticalScrollingEnabled()) {
                return list;
            }
            continue;
        }
        if (auto* mlist = dynamic_cast<Gui::MultiSelectList*>(w)) {
            if (mlist->IsVerticalScrollingEnabled()) {
                return mlist;
            }
            continue;
        }
        if (dynamic_cast<Gui::TreeView*>(w) != nullptr || dynamic_cast<Gui::ScrollPanel*>(w) != nullptr) {
            return w;
        }
    }
    return nullptr;
}

void SortCanvasesByOrder(Array<GuiCanvasComponent*>& list) {
    const std::size_t n = list.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        GuiCanvasComponent* key = list[i];
        std::size_t j = i;
        while (j > 0 && list[j - 1]->GetSortOrder() > key->GetSortOrder()) {
            list[j] = list[j - 1];
            --j;
        }
        list[j] = key;
    }
}

const Gui::Widget* FindTooltipSource(Gui::Widget* hit) noexcept {
    for (Gui::Widget* w = hit; w != nullptr; w = w->GetParent()) {
        if (!w->GetTooltip().IsEmpty()) {
            return w;
        }
    }
    return nullptr;
}

void PaintTooltipForWidget(Gui::GuiPaintContext& ctx, const Gui::Widget& w, float mouseX, float mouseY) {
    const Utf8String& tip = w.GetTooltip();
    if (tip.IsEmpty()) {
        return;
    }
    const Gui::GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float pad = m.Padding();
    const float maxW = 320.0F;
    float estW = std::min(maxW, static_cast<float>(tip.ByteLength()) * 7.5F + pad * 2.0F);
    estW = std::max(96.0F, estW);
    const float estH = m.FormRowHeight() + 6.0F;
    float tx = mouseX + 14.0F;
    float ty = mouseY + 14.0F;
    const float fbw = ctx.GetFramebufferWidth();
    const float fbh = ctx.GetFramebufferHeight();
    if (tx + estW > fbw) {
        tx = mouseX - estW - 8.0F;
    }
    if (ty + estH > fbh) {
        ty = mouseY - estH - 8.0F;
    }
    const Gui::GuiPointerState& ps = Gui::GetGuiPointerState();
    if (ps.tooltipClipRightPx > 0.0F) {
        if (tx + estW > ps.tooltipClipRightPx) {
            tx = ps.tooltipClipRightPx - estW - 8.0F;
        }
        tx = std::max(4.0F, tx);
        if (tx + estW > ps.tooltipClipRightPx) {
            estW = std::max(80.0F, ps.tooltipClipRightPx - tx - 4.0F);
        }
    }
    const Gui::GuiTheme& th = ctx.GetTheme();
    ctx.PushLateLayer();
    ctx.FillRoundRectGradientVertical(tx, ty, estW, estH, th.controlCornerRadius, th.panelElevatedTop,
            th.panelElevatedBottom, th.panelElevatedAlpha);
    ctx.StrokeRoundRect(tx, ty, estW, estH, th.controlCornerRadius, 1.0F, th.borderRgb, 0.65F);
    ctx.DrawText(tx + pad, ty + m.ControlGap() + 2.0F, m.FontBody(), tip, th.labelPrimary, 1.0F);
    ctx.PopLateLayer();
}

bool WidgetTreeHasVisibleDialog(const Gui::Widget* w) noexcept {
    if (w == nullptr || !w->IsVisible()) {
        return false;
    }
    if (dynamic_cast<const Gui::Dialog*>(w) != nullptr) {
        return true;
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (ch[i] && WidgetTreeHasVisibleDialog(ch[i].Get())) {
            return true;
        }
    }
    return false;
}

GuiCanvasComponent* FindCanvasWithVisibleDialog(const Array<GuiCanvasComponent*>& list) noexcept {
    for (std::size_t i = list.GetSize(); i > 0; --i) {
        GuiCanvasComponent* c = list[i - 1U];
        if (c != nullptr && c->IsCanvasEnabled() && WidgetTreeHasVisibleDialog(c->GetRoot())) {
            return c;
        }
    }
    return nullptr;
}

/** Open dropdown/list popups that extend outside clipped parents (e.g. ScrollPanel). */
Gui::Widget* FindOpenPopupHover(Gui::Widget* w, const float x, const float y) {
    if (w == nullptr || !w->IsVisible() || !w->IsEnabled()) {
        return nullptr;
    }
    if (auto* menu = dynamic_cast<Gui::MenuBar*>(w)) {
        if (menu->IsPopupOpen()) {
            if (Gui::Widget* hit = menu->FindDeepestHover(x, y)) {
                return hit;
            }
        }
    }
    if (auto* dropdown = dynamic_cast<Gui::Dropdown*>(w)) {
        if (dropdown->IsPopupOpen() && dropdown->HitPopupSurface(x, y)) {
            return dropdown;
        }
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (Gui::Widget* hit = FindOpenPopupHover(ch[i].Get(), x, y)) {
            return hit;
        }
    }
    return nullptr;
}

}  // namespace

void ProcessGuiCanvasesInput(GameWorld& world, IInput& input, int framebufferWidth, int framebufferHeight,
        const float contentScaleX, const float contentScaleY) {
    Gui::RefreshSystemDpiScale(contentScaleX, contentScaleY);

    if (!Gui::GuiToolkitSettings::ShouldProcessSparkGuiInput()) {
        Gui::GuiFrameInput fin{};
        input.GetCursorFramebufferPixels(fin.mouseX, fin.mouseY, framebufferWidth, framebufferHeight);
        Gui::GuiPointerState ptrState{};
        ptrState.mouseX = fin.mouseX;
        ptrState.mouseY = fin.mouseY;
        if (IImGuiLayer* imgui = GetActiveImGuiLayer(); imgui != nullptr && imgui->IsEnabled()) {
            ptrState.pointerOverGui = imgui->WantsCaptureMouse();
            ptrState.consumesGamePointer = imgui->WantsCaptureMouse() || imgui->WantsCaptureKeyboard();
        }
        Gui::SetGuiPointerState(ptrState);
        return;
    }

    static bool editorLayoutLoaded = false;
    if (!editorLayoutLoaded) {
        Gui::SceneEditorLayoutSettings layout{};
        Gui::TryLoadSceneEditorLayout(layout);
        editorLayoutLoaded = true;
    }

    Array<GuiCanvasComponent*> list;
    CollectCanvases(world, list);
    SortCanvasesByOrder(list);

    const float fbw = static_cast<float>(framebufferWidth > 0 ? framebufferWidth : 1);
    const float fbh = static_cast<float>(framebufferHeight > 0 ? framebufferHeight : 1);
    const Gui::Rect viewport{0.0F, 0.0F, fbw, fbh};

    const float effectiveScale = Gui::GetEffectiveGuiUiScale();
    Gui::GuiLayoutMetrics activeMetrics = Gui::GuiLayoutMetrics::Default();

    GuiCanvasComponent* modalCanvas = nullptr;
    for (std::size_t i = list.GetSize(); i > 0; --i) {
        GuiCanvasComponent* c = list[i - 1U];
        if (c != nullptr && c->IsCanvasEnabled() && c->GetModalInputCapture()) {
            modalCanvas = c;
            break;
        }
    }
    if (modalCanvas == nullptr) {
        modalCanvas = FindCanvasWithVisibleDialog(list);
    }

    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        GuiCanvasComponent* c = list[i];
        if (c == nullptr || !c->IsCanvasEnabled()) {
            continue;
        }
        if (modalCanvas != nullptr && c != modalCanvas) {
            continue;
        }
        Gui::GuiLayoutMetrics canvasMetrics = c->GetLayoutMetrics();
        canvasMetrics.uiScale = effectiveScale;
        c->SetLayoutMetrics(canvasMetrics);
        if (Gui::Widget* r = c->GetRoot()) {
            r->Arrange(viewport);
        }
    }
    activeMetrics.uiScale = effectiveScale;
    Gui::SetActiveGuiLayoutMetrics(activeMetrics);
    if (modalCanvas != nullptr) {
        activeMetrics = modalCanvas->GetLayoutMetrics();
        activeMetrics.uiScale = effectiveScale;
        Gui::SetActiveGuiLayoutMetrics(activeMetrics);
    } else if (list.GetSize() > 0) {
        for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
            GuiCanvasComponent* c = list[ri - 1U];
            if (c != nullptr && c->IsCanvasEnabled()) {
                activeMetrics = c->GetLayoutMetrics();
                activeMetrics.uiScale = effectiveScale;
                Gui::SetActiveGuiLayoutMetrics(activeMetrics);
                break;
            }
        }
    }

    Gui::GuiFrameInput fin{};
    input.GetCursorFramebufferPixels(fin.mouseX, fin.mouseY, framebufferWidth, framebufferHeight);
    fin.leftDown = input.IsMouseButtonDown(0);
    fin.leftPressedThisFrame = input.IsMouseButtonPressedThisFrame(0);
    fin.leftReleasedThisFrame = input.IsMouseButtonReleasedThisFrame(0);
    fin.rightDown = input.IsMouseButtonDown(1);
    fin.rightPressedThisFrame = input.IsMouseButtonPressedThisFrame(1);
    fin.scrollDeltaY = input.GetScrollDeltaY();
    fin.ctrlDown = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
    fin.shiftDown = input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
    fin.altDown = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);

    Gui::GuiPointerState ptrState{};
    ptrState.mouseX = fin.mouseX;
    ptrState.mouseY = fin.mouseY;
    ptrState.contextMenuOpen = Gui::GetGuiContextMenu().IsOpen();

    if (ptrState.contextMenuOpen) {
        GuiCanvasComponent* menuOwner = nullptr;
        for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
            GuiCanvasComponent* c = list[ri - 1U];
            if (c != nullptr && c->IsCanvasEnabled() && c->GetRoot() != nullptr) {
                menuOwner = c;
                break;
            }
        }
        if (menuOwner != nullptr) {
            Gui::GetGuiContextMenu().HandlePointer(fin, *menuOwner);
        }
        ptrState.pointerOverGui = true;
        ptrState.consumesGamePointer = true;
        Gui::SetGuiPointerState(ptrState);
        return;
    }

    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        list[i]->ClearTransientPointerState();
    }

    GuiCanvasComponent* pointerOwner = nullptr;
    Gui::Widget* hotWidget = nullptr;
    for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
        GuiCanvasComponent* c = list[ri - 1U];
        if (c == nullptr || !c->IsCanvasEnabled()) {
            continue;
        }
        if (modalCanvas != nullptr && c != modalCanvas) {
            continue;
        }
        Gui::Widget* r = c->GetRoot();
        if (r == nullptr || !r->IsVisible()) {
            continue;
        }
        if (Gui::Widget* popupHit = FindOpenPopupHover(r, fin.mouseX, fin.mouseY)) {
            pointerOwner = c;
            hotWidget = popupHit;
            break;
        }
    }
    if (hotWidget == nullptr) {
        for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
            GuiCanvasComponent* c = list[ri - 1U];
            if (c == nullptr || !c->IsCanvasEnabled()) {
                continue;
            }
            if (modalCanvas != nullptr && c != modalCanvas) {
                continue;
            }
            Gui::Widget* r = c->GetRoot();
            if (r == nullptr || !r->IsVisible()) {
                continue;
            }
            if (Gui::Widget* hit = r->FindDeepestHover(fin.mouseX, fin.mouseY)) {
                pointerOwner = c;
                hotWidget = hit;
                break;
            }
        }
    }

    ptrState.pointerOverGui = hotWidget != nullptr;
    ptrState.tooltipSource = FindTooltipSource(hotWidget);

    GuiCanvasComponent* captureCanvas = nullptr;
    for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
        GuiCanvasComponent* c = list[ri - 1U];
        if (c != nullptr && c->IsCanvasEnabled() && c->GetActivePressWidget() != nullptr) {
            captureCanvas = c;
            break;
        }
    }

    GuiCanvasComponent* stepCanvas = pointerOwner != nullptr ? pointerOwner : captureCanvas;
    bool scrollWheelConsumed = false;
    if (stepCanvas != nullptr) {
        if (fin.scrollDeltaY != 0.0F) {
            Gui::Widget* scrollTarget = nullptr;
            if (pointerOwner != nullptr) {
                scrollTarget = FindScrollableAncestor(hotWidget);
                if (scrollTarget == nullptr) {
                    scrollTarget = FindTopmostScrollableUnderPoint(pointerOwner->GetRoot(), fin.mouseX, fin.mouseY);
                }
            } else if (captureCanvas != nullptr) {
                scrollTarget = FindTopmostScrollableUnderPoint(captureCanvas->GetRoot(), fin.mouseX, fin.mouseY);
            }
            if (scrollTarget != nullptr) {
                if (auto* hoverList = dynamic_cast<Gui::List*>(scrollTarget)) {
                    hoverList->ApplyScrollWheelDelta(fin.scrollDeltaY);
                    scrollWheelConsumed = true;
                } else if (auto* mlist = dynamic_cast<Gui::MultiSelectList*>(scrollTarget)) {
                    mlist->ApplyScrollWheelDelta(fin.scrollDeltaY);
                    scrollWheelConsumed = true;
                } else if (auto* tree = dynamic_cast<Gui::TreeView*>(scrollTarget)) {
                    tree->ApplyScrollWheelDelta(fin.scrollDeltaY);
                    scrollWheelConsumed = true;
                } else if (auto* panel = dynamic_cast<Gui::ScrollPanel*>(scrollTarget)) {
                    panel->ApplyScrollWheelDelta(fin.scrollDeltaY);
                    scrollWheelConsumed = true;
                }
            }
        }
        if (pointerOwner == stepCanvas && hotWidget != nullptr) {
            stepCanvas->SetFrameHotWidget(hotWidget);
        }
        stepCanvas->StepPointer(fin);
        if (fin.leftPressedThisFrame) {
            for (std::size_t i = 0; i < list.GetSize(); ++i) {
                GuiCanvasComponent* c = list[i];
                if (c != nullptr && c != stepCanvas && c->IsCanvasEnabled()) {
                    c->ClearTransientPointerState();
                    c->ClearKeyboardFocus();
                }
            }
        }
        if (Gui::Widget* r = stepCanvas->GetRoot()) {
            r->Arrange(viewport);
        }
    }

    if (fin.leftPressedThisFrame) {
        for (std::size_t i = 0; i < list.GetSize(); ++i) {
            GuiCanvasComponent* c = list[i];
            if (c == nullptr || !c->IsCanvasEnabled()) {
                continue;
            }
            if (modalCanvas != nullptr && c != modalCanvas) {
                continue;
            }
            Gui::Widget* root = c->GetRoot();
            if (root == nullptr) {
                continue;
            }
            Gui::Widget* canvasHit = c == pointerOwner ? hotWidget
                    : (c == stepCanvas ? stepCanvas->GetRoot()->FindDeepestHover(fin.mouseX, fin.mouseY)
                                       : root->FindDeepestHover(fin.mouseX, fin.mouseY));
            Gui::DismissWidgetPopups(root, fin, canvasHit);
        }
    }

    Gui::HandleGlobalPopupKeys(input, modalCanvas != nullptr ? modalCanvas->GetRoot() : (pointerOwner != nullptr ? pointerOwner->GetRoot() : nullptr));

    ptrState.consumesGamePointer = ptrState.pointerOverGui
            || (captureCanvas != nullptr && fin.leftDown);
    ptrState.scrollWheelConsumed = scrollWheelConsumed;

    if (IImGuiLayer* imgui = GetActiveImGuiLayer(); imgui != nullptr && imgui->IsEnabled()) {
        if (imgui->WantsCaptureMouse()) {
            ptrState.pointerOverGui = true;
            ptrState.consumesGamePointer = true;
        }
        if (imgui->WantsCaptureKeyboard()) {
            ptrState.consumesGamePointer = true;
        }
    }

    GuiCanvasComponent* keyCanvas = nullptr;
    if (modalCanvas != nullptr) {
        keyCanvas = modalCanvas;
    } else {
        for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
            GuiCanvasComponent* c = list[ri - 1U];
            if (c != nullptr && c->GetFocusWidget() != nullptr) {
                keyCanvas = c;
                break;
            }
        }
    }
    if (keyCanvas == nullptr && pointerOwner != nullptr && pointerOwner->GetFocusWidget() != nullptr) {
        keyCanvas = pointerOwner;
    }
    if (keyCanvas != nullptr) {
        keyCanvas->ProcessKeyFocus(input);
    } else if (!list.IsEmpty() && input.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
        GuiCanvasComponent* target = pointerOwner;
        if (target == nullptr || !target->IsCanvasEnabled()) {
            for (std::size_t ri = list.GetSize(); ri > 0; --ri) {
                GuiCanvasComponent* c = list[ri - 1U];
                if (c != nullptr && c->IsCanvasEnabled() && c->GetRoot() != nullptr) {
                    target = c;
                    break;
                }
            }
        }
        if (target != nullptr) {
            target->ProcessKeyFocus(input);
        }
    }

    /** Popups may open/close during pointer handling — sync layout before paint. */
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        GuiCanvasComponent* c = list[i];
        if (c == nullptr || !c->IsCanvasEnabled()) {
            continue;
        }
        if (modalCanvas != nullptr && c != modalCanvas) {
            continue;
        }
        if (Gui::Widget* r = c->GetRoot()) {
            r->Arrange(viewport);
        }
    }

    Gui::SetGuiPointerState(ptrState);
}

void ProcessGuiCanvasesInput(Scene& scene, IInput& input, int framebufferWidth, int framebufferHeight,
        const float contentScaleX, const float contentScaleY) {
    ProcessGuiCanvasesInput(scene.GetWorld(), input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void PaintGuiCanvases(const GameWorld& world, SceneRenderParams& params, int framebufferWidth, int framebufferHeight) {
    Array<GuiCanvasComponent*> list;
    CollectCanvases(world, list);
    SortCanvasesByOrder(list);

    const float fbw = static_cast<float>(framebufferWidth > 0 ? framebufferWidth : 1);
    const float fbh = static_cast<float>(framebufferHeight > 0 ? framebufferHeight : 1);
    const Gui::Rect viewport{0.0F, 0.0F, fbw, fbh};

    Gui::GuiPaintContext ctx(params);
    ctx.SetFramebufferPixelSize(fbw, fbh);
    if (const SharedPtr<Font>& uiFont = world.GetUiFont(); uiFont) {
        ctx.SetLayoutFont(uiFont.Get());
    }
    Gui::GuiLayoutMetrics paintMetrics = Gui::GuiLayoutMetrics::Default();
    const float effectiveScale = Gui::GetEffectiveGuiUiScale();
    paintMetrics.uiScale = effectiveScale;
    Gui::SetActiveGuiLayoutMetrics(paintMetrics);
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        GuiCanvasComponent* c = list[i];
        if (c == nullptr || !c->IsCanvasEnabled()) {
            continue;
        }
        Gui::Widget* r = c->GetRoot();
        if (r == nullptr || !r->IsVisible()) {
            continue;
        }
        Gui::GuiLayoutMetrics canvasMetrics = c->GetLayoutMetrics();
        canvasMetrics.uiScale = effectiveScale;
        paintMetrics = canvasMetrics;
        Gui::SetActiveGuiLayoutMetrics(paintMetrics);
        r->Arrange(viewport);
        ctx.ResetOverlayLayer();
        ctx.ResetLateLayer();
        ctx.SetTheme(&c->GetTheme());
        ctx.SetSkin(c->GetSkinMutable());
        if (Gui::GuiSkin* skin = c->GetSkinMutable()) {
            skin->PrimeUiTextures(ctx);
        }
        c->Paint(ctx);
        if (Gui::Widget* root = c->GetRoot()) {
            Gui::PaintOpenDropdownPopups(root, ctx);
        }
    }

    if (Gui::GetGuiContextMenu().IsOpen()) {
        Gui::GetGuiContextMenu().Paint(ctx);
    }

    const Gui::GuiPointerState& ps = Gui::GetGuiPointerState();
    if (ps.showTooltip && ps.tooltipSource != nullptr) {
        PaintTooltipForWidget(ctx, *ps.tooltipSource, ps.mouseX, ps.mouseY);
    }
}

void PaintGuiCanvases(const Scene& scene, SceneRenderParams& params, int framebufferWidth, int framebufferHeight) {
    PaintGuiCanvases(scene.GetWorld(), params, framebufferWidth, framebufferHeight);
}

}  // namespace Spark
