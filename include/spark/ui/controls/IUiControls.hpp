#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ui/core/IUiElement.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark::Ui {

class IUiButton : public virtual IUiElement {
public:
    virtual void SetLabel(Utf8String label) = 0;
    [[nodiscard]] virtual Utf8StringView GetLabel() const = 0;
    virtual void SetOnClick(UiVoidCallback handler) = 0;
    [[nodiscard]] virtual bool WasClickedThisFrame() const = 0;
};

class ISlider : public virtual IUiElement {
public:
    virtual void SetValue(float value) = 0;
    [[nodiscard]] virtual float GetValue() const = 0;
    virtual void SetRange(float minValue, float maxValue) = 0;
    virtual void SetOnChanged(UiFloatCallback handler) = 0;
};

class ICheckBox : public virtual IUiElement {
public:
    virtual void SetValue(bool value) = 0;
    [[nodiscard]] virtual bool GetValue() const = 0;
    virtual void SetOnChanged(UiBoolCallback handler) = 0;
};

class IPanel : public virtual IUiElement {
public:
    virtual void SetTitle(Utf8String title) = 0;
    [[nodiscard]] virtual bool IsOpen() const = 0;
};

class ILabel : public virtual IUiElement {
public:
    virtual void SetText(Utf8String text) = 0;
    virtual void SetMuted(bool muted) = 0;
};

class ISeparator : public virtual IUiElement {};

class IScrollPanel : public virtual IUiElement {
public:
    virtual void SetScrollY(float y) = 0;
    [[nodiscard]] virtual float GetScrollY() const = 0;
    virtual void ScrollToTop() = 0;
};

class IDockWorkspace : public virtual IUiElement {
public:
    virtual IUiElement* GetLeftPane() noexcept = 0;
    virtual IUiElement* GetCenterPane() noexcept = 0;
    virtual IUiElement* GetRightPane() noexcept = 0;

    virtual void ToggleLeftCollapsed() noexcept {}
    virtual void ToggleRightCollapsed() noexcept {}
    [[nodiscard]] virtual Rect GetCenterBounds() const noexcept { return {}; }
    virtual void SetLeftWidth(float width) noexcept { (void)width; }
    [[nodiscard]] virtual float GetLeftWidth() const noexcept { return 0.0F; }
    virtual void SetRightWidth(float width) noexcept { (void)width; }
    [[nodiscard]] virtual float GetRightWidth() const noexcept { return 0.0F; }
};

class IList : public virtual IUiElement {
public:
    virtual void SetItems(Array<Utf8String> items) = 0;
    [[nodiscard]] virtual int GetSelectedIndex() const = 0;
    virtual void SetSelectedIndex(int index) = 0;
    virtual void SetOnSelectionChanged(UiIntCallback handler) = 0;
    virtual void SetScrollY(float y) = 0;
    [[nodiscard]] virtual float GetScrollY() const = 0;
    virtual void ScrollToTop() = 0;
};

class IMultiSelectList : public virtual IUiElement {
public:
    virtual void SetItems(Array<Utf8String> items) = 0;
    [[nodiscard]] virtual const Array<int>& GetSelectedIndices() const = 0;
    virtual void SetSelectedIndices(Array<int> indices) = 0;
    virtual void SetOnSelectionChanged(UiVoidCallback handler) = 0;
    virtual void SetScrollY(float y) = 0;
    [[nodiscard]] virtual float GetScrollY() const = 0;
};

class ITreeView : public virtual IUiElement {
public:
    virtual void Clear() = 0;
    /** Returns new node index, or -1 if parent is invalid. */
    virtual int AddItem(int parentIndex, Utf8String label) = 0;
    [[nodiscard]] virtual int GetSelectedNodeId() const = 0;
    virtual void SetOnSelectionChanged(UiIntCallback handler) = 0;
};

using IButton = IUiButton;

}  // namespace Spark::Ui
