#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

enum class DockOrientation {
    Horizontal,
    Vertical,
};

/**
 * Static two-pane dock: first / second child by creation order. Use <c>Splitter</c> for draggable splits,
 * or <c>DockWorkspace</c> for a full tabbed/split layout model.
 */
class DockLayout final : public Widget {
public:
    void SetOrientation(DockOrientation o) noexcept { orientation = o; }
    void SetFirstFraction(float t) noexcept {
        if (t < 0.05F) {
            firstFrac = 0.05F;
        } else if (t > 0.95F) {
            firstFrac = 0.95F;
        } else {
            firstFrac = t;
        }
    }
    [[nodiscard]] float GetFirstFraction() const noexcept { return firstFrac; }

    void Arrange(const Rect& r) override;

private:
    DockOrientation orientation = DockOrientation::Horizontal;
    float firstFrac = 0.5F;
};

}  // namespace Spark::Gui
