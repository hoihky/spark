#pragma once

namespace Spark::Gui {

/**
 * Written each frame by <c>ProcessGuiCanvasesInput</c> after layout and pointer routing.
 * Games should use this instead of hard-coded UI strip widths to decide whether 3D picking runs.
 */
struct GuiPointerState {
    float mouseX = 0.0F;
    float mouseY = 0.0F;
    /** Cursor is over a visible, hit-testable widget on any enabled canvas (topmost wins for routing). */
    bool pointerOverGui = false;
    /**
     * When true, gameplay should not use the cursor for world interaction (same as pointerOverGui today;
     * reserved for future passthrough viewports inside a canvas).
     */
    bool consumesGamePointer = false;
    /** Set when the wheel delta was applied to a scrollable widget this frame. */
    bool scrollWheelConsumed = false;
    /** A modal context menu is open; blocks game pointer until dismissed. */
    bool contextMenuOpen = false;
    /** Widget supplying tooltip text this frame (may be null). */
    const class Widget* tooltipSource = nullptr;
    /** False until the cursor has hovered the same tooltip source for <c>kTooltipDelayFrames</c>. */
    bool showTooltip = false;
    /**
     * When &gt; 0, tooltips are shifted left so they do not extend past this X (framebuffer pixels).
     * Scene editor sets this to the left-pane right edge so tooltips do not cover the splitter gutter.
     */
    float tooltipClipRightPx = -1.0F;
};

[[nodiscard]] const GuiPointerState& GetGuiPointerState() noexcept;

/** Updated by <c>ProcessGuiCanvasesInput</c> each frame. */
void SetGuiPointerState(const GuiPointerState& state) noexcept;

/** Merges tooltip clip into the current pointer state (e.g. scene editor before paint). */
void SetTooltipClipRightPx(float rightPx) noexcept;

}  // namespace Spark::Gui
