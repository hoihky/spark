#include "spark/gui/GuiWidgetPopups.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiContextMenu.hpp"
#include "spark/gui/controls/Dropdown.hpp"
#include "spark/gui/controls/MenuBar.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/Widget.hpp"

#include <GLFW/glfw3.h>

namespace Spark::Gui {

namespace {

void DismissPopupsRecursive(Widget* w, const GuiFrameInput& in, Widget* hitWidget) noexcept {
    if (w == nullptr || !w->IsVisible()) {
        return;
    }
    if (auto* dropdown = dynamic_cast<Dropdown*>(w)) {
        if (dropdown->IsPopupOpen() && in.leftPressedThisFrame) {
            if (!dropdown->HitPopupSurface(in.mouseX, in.mouseY)) {
                dropdown->ClosePopup();
            }
        }
    }
    if (auto* menu = dynamic_cast<MenuBar*>(w)) {
        if (in.leftPressedThisFrame) {
            menu->DismissPopupUnlessHit(in.mouseX, in.mouseY, hitWidget);
        }
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (ch[i]) {
            DismissPopupsRecursive(ch[i].Get(), in, hitWidget);
        }
    }
}

void CloseAllDropdownsRecursive(Widget* w) noexcept {
    if (w == nullptr) {
        return;
    }
    if (auto* dropdown = dynamic_cast<Dropdown*>(w)) {
        dropdown->ClosePopup();
    }
    if (auto* menu = dynamic_cast<MenuBar*>(w)) {
        menu->ClosePopup();
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (ch[i]) {
            CloseAllDropdownsRecursive(ch[i].Get());
        }
    }
}

void PaintOpenDropdownPopupsRecursive(const Widget* w, GuiPaintContext& ctx) {
    if (w == nullptr || !w->IsVisible()) {
        return;
    }
    if (const auto* dropdown = dynamic_cast<const Dropdown*>(w)) {
        /** Popup subtree is painted inside PaintOpenPopup — do not descend or it repaints on the main layer. */
        dropdown->PaintOpenPopup(ctx);
        return;
    }
    const auto& ch = w->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        if (ch[i]) {
            PaintOpenDropdownPopupsRecursive(ch[i].Get(), ctx);
        }
    }
}

}  // namespace

bool IsDescendantOf(const Widget* node, const Widget* ancestor) noexcept {
    if (node == nullptr || ancestor == nullptr) {
        return false;
    }
    for (const Widget* p = node; p != nullptr; p = p->GetParent()) {
        if (p == ancestor) {
            return true;
        }
    }
    return false;
}

void PaintOpenDropdownPopups(const Widget* root, GuiPaintContext& ctx) {
    PaintOpenDropdownPopupsRecursive(root, ctx);
}

void DismissWidgetPopups(Widget* root, const GuiFrameInput& in, Widget* hitWidget) noexcept {
    if (root == nullptr) {
        return;
    }
    DismissPopupsRecursive(root, in, hitWidget);
}

void HandleGlobalPopupKeys(IInput& input, Widget* rootOnModalCanvas) noexcept {
    if (!input.IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
        return;
    }
    if (GetGuiContextMenu().IsOpen()) {
        GetGuiContextMenu().Close();
        return;
    }
    if (rootOnModalCanvas != nullptr) {
        CloseAllDropdownsRecursive(rootOnModalCanvas);
    }
}

}  // namespace Spark::Gui
