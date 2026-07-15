#include "spark/gui/Widget.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/GuiPaintContext.hpp"

namespace Spark::Gui {

Widget::~Widget() {
    ClearChildren();
}

void Widget::RemoveChild(Widget* child) {
    if (child == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i].Get() == child) {
            child->parent = nullptr;
            children.RemoveAt(i);
            return;
        }
    }
}

void Widget::ClearChildren() {
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i]) {
            children[i]->parent = nullptr;
        }
    }
    children.Clear();
}

void Widget::Arrange(const Rect& r) {
    bounds = r;
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i]) {
            children[i]->Arrange(r);
        }
    }
}

Widget* Widget::FindDeepestHover(float x, float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    for (std::size_t i = children.GetSize(); i > 0; --i) {
        Widget* c = children[i - 1U].Get();
        if (c == nullptr || !c->IsVisible()) {
            continue;
        }
        if (Widget* hit = c->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (hitTest && bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void Widget::NotifyPointerDown(const GuiFrameInput& /*in*/, GuiCanvasComponent& /*canvas*/) {}

void Widget::NotifyPointerUp(const GuiFrameInput& /*in*/, GuiCanvasComponent& /*canvas*/) {}

void Widget::NotifyPointerDrag(const GuiFrameInput& /*in*/, GuiCanvasComponent& /*canvas*/) {}

void Widget::NotifyClick(const GuiFrameInput& /*in*/, GuiCanvasComponent& /*canvas*/) {}

void Widget::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    PaintChildren(ctx);
}

void Widget::ProcessKeyInput(IInput& /*input*/) {}

void Widget::PaintChildren(GuiPaintContext& ctx) const {
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] && children[i]->IsVisible()) {
            children[i]->Paint(ctx);
        }
    }
}

}  // namespace Spark::Gui
