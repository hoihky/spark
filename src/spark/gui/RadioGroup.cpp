#include "spark/gui/RadioGroup.hpp"

#include "spark/gui/controls/RadioButton.hpp"

namespace Spark::Gui {

void RadioGroup::Register(RadioButton* b) {
    if (b == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < buttons.GetSize(); ++i) {
        if (buttons[i] == b) {
            return;
        }
    }
    buttons.PushBack(b);
}

void RadioGroup::Unregister(RadioButton* b) {
    if (b == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < buttons.GetSize(); ++i) {
        if (buttons[i] == b) {
            buttons.RemoveAt(i);
            if (selected == b) {
                selected = nullptr;
            }
            return;
        }
    }
}

void RadioGroup::Select(RadioButton* b) noexcept {
    if (selected == b) {
        return;
    }
    RadioButton* old = selected;
    selected = b;
    if (old != nullptr) {
        old->ApplyGroupSelection(false);
    }
    if (selected != nullptr) {
        selected->ApplyGroupSelection(true);
    }
}

}  // namespace Spark::Gui
