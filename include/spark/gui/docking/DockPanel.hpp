#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Gui {

enum class DockSide {
    Left,
    Right,
    Center,
};

/**
 * Logical dock panel: stable id, display title, owned content widget, and preferred dock side.
 * Registered with <c>DockManager</c> which places panels into side panes or tabs.
 */
class DockPanel {
public:
    DockPanel(Utf8String id, Utf8String title, UniquePtr<Widget> content, DockSide side = DockSide::Left);

    [[nodiscard]] const Utf8String& GetId() const noexcept { return id; }
    [[nodiscard]] const Utf8String& GetTitle() const noexcept { return title; }
    [[nodiscard]] DockSide GetSide() const noexcept { return side; }
    [[nodiscard]] Widget* GetContent() noexcept { return content.Get(); }
    [[nodiscard]] const Widget* GetContent() const noexcept { return content.Get(); }
    [[nodiscard]] UniquePtr<Widget> ReleaseContent() { return MoveTemp(content); }

private:
    Utf8String id;
    Utf8String title;
    UniquePtr<Widget> content;
    DockSide side = DockSide::Left;
};

}  // namespace Spark::Gui
