#include "spark/gui/docking/DockPanel.hpp"

namespace Spark::Gui {

DockPanel::DockPanel(
        Utf8String id,
        Utf8String title,
        UniquePtr<Widget> content,
        const DockSide side)
    : id_(MoveTemp(id)), title_(MoveTemp(title)), content_(MoveTemp(content)), side_(side) {}

}  // namespace Spark::Gui
