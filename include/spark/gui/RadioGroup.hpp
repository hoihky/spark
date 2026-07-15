#pragma once

#include "spark/core/Array.hpp"

namespace Spark::Gui {

class RadioButton;

/**
 * Mutual exclusion for <c>RadioButton</c> widgets. Register each button; clicking one clears the others.
 * Lifetime: buttons should unregister (or be destroyed after clearing the group) — <c>RadioButton</c>
 * unregisters from its group in its destructor when <c>SetGroup</c> was used.
 */
class RadioGroup {
public:
    void Register(RadioButton* b);
    void Unregister(RadioButton* b);
    /** Programmatically select one button (clears others); pass nullptr to clear all. */
    void Select(RadioButton* b) noexcept;

    [[nodiscard]] RadioButton* GetSelected() const noexcept { return selected; }

private:
    Array<RadioButton*> buttons{};
    RadioButton* selected = nullptr;
};

}  // namespace Spark::Gui
