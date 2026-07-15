#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

enum class StackOrientation {
    Vertical,
    Horizontal,
};

/** Stacks children in a row or column with uniform spacing.
 *  Vertical mode divides height equally — do not use for multi-row forms; use <c>VStackForm</c> instead. */
class StackPanel final : public Widget {
public:
    void SetOrientation(StackOrientation o) noexcept { orientation = o; }
    void SetSpacing(float s) noexcept { spacing = s; }

    void Arrange(const Rect& r) override;

private:
    StackOrientation orientation = StackOrientation::Vertical;
    float spacing = 8.0F;
};

}  // namespace Spark::Gui
