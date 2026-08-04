#pragma once

#include "spark/ui/core/IUiElement.hpp"

namespace Spark::Ui {

/**
 * Shared tree plumbing for Spark and ImGui control implementations.
 * Subclasses implement <c>DoMeasure</c>, <c>DoPaint</c>, and optional input overrides.
 */
class UiElementBase : public virtual IUiElement {
public:
    explicit UiElementBase(UiElementId idIn) noexcept;
    ~UiElementBase() override = default;

    [[nodiscard]] UiElementId GetId() const noexcept override { return id; }
    [[nodiscard]] Rect GetBounds() const noexcept override { return bounds; }

    void Measure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;

    [[nodiscard]] IUiElement* HitTest(const float x, const float y) override;
    [[nodiscard]] const IUiElement* HitTest(const float x, const float y) const override;

    [[nodiscard]] bool IsVisible() const noexcept override { return visible; }
    [[nodiscard]] bool IsEnabled() const noexcept override { return enabled; }
    [[nodiscard]] bool WantsHitTest() const noexcept override { return hitTest && visible && enabled; }

    [[nodiscard]] IUiElement* GetParent() noexcept override { return parent; }
    [[nodiscard]] const IUiElement* GetParent() const noexcept override { return parent; }

    void SetVisible(const bool v) noexcept { visible = v; }
    void SetEnabled(const bool e) noexcept { enabled = e; }
    void SetHitTest(const bool h) noexcept { hitTest = h; }

    void AddChild(UniquePtr<IUiElement> child) override;
    [[nodiscard]] const Array<UniquePtr<IUiElement>>& GetChildren() const noexcept override { return children; }

protected:
    virtual void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired);
    virtual void DoPaint(IUiRenderer& renderer);
    virtual void DoArrangeChildren();

    UiElementId id{};
    Rect bounds{};
    bool visible = true;
    bool enabled = true;
    bool hitTest = true;
    IUiElement* parent = nullptr;
    Array<UniquePtr<IUiElement>> children{};
};

}  // namespace Spark::Ui
