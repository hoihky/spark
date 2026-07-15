#pragma once

#include "spark/gui/GuiTypes.hpp"

namespace Spark {

class IInput;

namespace Gui {

class GuiPaintContext;
class Widget;

/** Paints open Dropdown popups on the late layer after the main widget tree. */
void PaintOpenDropdownPopups(const Widget* root, GuiPaintContext& ctx);

/** Closes open Dropdown / MenuBar popups when the pointer hits outside them. */
void DismissWidgetPopups(Widget* root, const GuiFrameInput& in, Widget* hitWidget) noexcept;

/** Escape and global dismiss for context menu + in-tree popups. */
void HandleGlobalPopupKeys(IInput& input, Widget* rootOnModalCanvas) noexcept;

[[nodiscard]] bool IsDescendantOf(const Widget* node, const Widget* ancestor) noexcept;

}  // namespace Gui
}  // namespace Spark
