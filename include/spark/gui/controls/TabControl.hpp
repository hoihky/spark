#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/TypeTraits.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <functional>

namespace Spark::Gui {

class Panel;
class GuiPaintContext;

/** Tab strip + single visible page panel; each page is a child of the body panel. */
class TabControl final : public Widget {
public:
    TabControl();

    template<typename T>
        requires DerivedFrom<T, Widget>
    void AddTab(Utf8String title, UniquePtr<T> page) {
        AddTabImpl(Spark::MoveTemp(title), UniquePtr<Widget>(static_cast<Widget*>(page.Release())));
    }

    void AddTabWidget(Utf8String title, UniquePtr<Widget> page) {
        AddTabImpl(Spark::MoveTemp(title), MoveTemp(page));
    }

    void SetSelectedIndex(int i);
    [[nodiscard]] int GetSelectedIndex() const noexcept { return selected; }
    void SetTabBarHeight(float h) noexcept { tabBarH = h; }
    void SetOnTabChanged(std::function<void(int index)> fn) { onTabChanged = Spark::MoveTemp(fn); }
    [[nodiscard]] Widget* Body() noexcept { return body; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    void AddTabImpl(Utf8String title, UniquePtr<Widget> page);
    void Select(int i);

    float tabBarH = 38.0F;
    int selected = 0;
    Array<Utf8String> titles{};
    Panel* header = nullptr;
    Widget* body = nullptr;
    std::function<void(int index)> onTabChanged{};
};

}  // namespace Spark::Gui
