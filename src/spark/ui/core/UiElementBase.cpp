#include "spark/ui/core/UiElementBase.hpp"

#include "spark/ui/core/IUiRenderer.hpp"

#include <algorithm>

namespace Spark::Ui {

UiElementBase::UiElementBase(UiElementId idIn) noexcept : id(MoveTemp(idIn)) {}

void UiElementBase::Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    DoMeasure(constraints, outDesired);
}

void UiElementBase::Arrange(const Rect& finalBounds) {
    bounds = finalBounds;
    DoArrangeChildren();
}

void UiElementBase::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    DoPaint(renderer);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Paint(renderer);
        }
    }
}

IUiElement* UiElementBase::HitTest(const float x, const float y) {
    if (!WantsHitTest() || !bounds.Contains(x, y)) {
        return nullptr;
    }
    for (std::size_t ci = children.GetSize(); ci > 0; --ci) {
        IUiElement* const child = children[ci - 1U].Get();
        if (child == nullptr) {
            continue;
        }
        if (IUiElement* hit = child->HitTest(x, y)) {
            return hit;
        }
    }
    return this;
}

const IUiElement* UiElementBase::HitTest(const float x, const float y) const {
    return const_cast<UiElementBase*>(this)->HitTest(x, y);
}

void UiElementBase::AddChild(UniquePtr<IUiElement> child) {
    if (child) {
        if (auto* baseChild = dynamic_cast<UiElementBase*>(child.Get())) {
            baseChild->parent = this;
        }
        children.PushBack(MoveTemp(child));
    }
}

void UiElementBase::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    outDesired.width = std::max(constraints.minWidth, 1.0F);
    outDesired.height = std::max(constraints.minHeight, 1.0F);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        UiSize childDesired{};
        children[i]->Measure(constraints, childDesired);
        outDesired.width = std::max(outDesired.width, childDesired.width);
        outDesired.height = std::max(outDesired.height, childDesired.height);
    }
}

void UiElementBase::DoPaint(IUiRenderer& /*renderer*/) {}

void UiElementBase::DoArrangeChildren() {
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            children[i]->Arrange(bounds);
        }
    }
}

}  // namespace Spark::Ui
