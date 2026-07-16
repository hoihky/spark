#include "spark/gui/docking/DockPanel.hpp"

namespace Spark::Gui {

DockPanel::DockPanel(
        Utf8String id,
        Utf8String title,
        UniquePtr<Widget> content,
        const DockSide side)
    : id(MoveTemp(id)), title(MoveTemp(title)), content(MoveTemp(content)), side(side) {}

}  // namespace Spark::Gui
