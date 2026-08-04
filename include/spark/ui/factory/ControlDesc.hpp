#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark::Ui {

struct ButtonDesc {
    UiElementId id{};
    Utf8String label{};
    bool enabled = true;
};

struct SliderDesc {
    UiElementId id{};
    Utf8String label{};
    float value = 0.0F;
    float minValue = 0.0F;
    float maxValue = 1.0F;
    bool enabled = true;
};

struct CheckBoxDesc {
    UiElementId id{};
    Utf8String label{};
    bool value = false;
    bool enabled = true;
};

struct PanelDesc {
    UiElementId id{};
    Utf8String title{};
    bool collapsible = false;
    bool open = true;
    /** Design width in framebuffer pixels before <c>uiScale</c>; 0 fills parent. */
    float width = 0.0F;
    /** Design height; 0 uses parent height minus margins. */
    float height = 0.0F;
    bool anchorRight = false;
    float edgeMargin = 8.0F;
    /** When true, centers the panel within the parent's arranged bounds. */
    bool centerInParent = false;
};

struct LabelDesc {
    UiElementId id{};
    Utf8String text{};
    bool muted = false;
};

struct SeparatorDesc {
    UiElementId id{};
};

struct ScrollPanelDesc {
    UiElementId id{};
    /** Design height before <c>uiScale</c>. */
    float height = 240.0F;
    /** 0 uses <c>UiLayoutMetrics::FormRowHeight</c>. */
    float rowHeight = 0.0F;
    float verticalGap = 4.0F;
};

struct DockWorkspaceDesc {
    UiElementId id{};
    float leftWidth = 280.0F;
    float rightWidth = 320.0F;
};

struct ListDesc {
    UiElementId id{};
    /** 0 uses <c>UiLayoutMetrics::ListRowHeight</c>. */
    float rowHeight = 0.0F;
    float itemFontSize = 0.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    bool verticalScrollingEnabled = true;
    /** Dear ImGui: expand the list box to use remaining panel height (last control in a panel). */
    bool fillRemainingHeight = false;
};

struct MultiSelectListDesc {
    UiElementId id{};
    float rowHeight = 0.0F;
    float itemFontSize = 0.0F;
    bool itemBold = false;
    bool opaqueRows = false;
    bool verticalScrollingEnabled = true;
};

struct TreeViewDesc {
    UiElementId id{};
    float rowHeight = 0.0F;
    float itemFontSize = 0.0F;
};

}  // namespace Spark::Ui
