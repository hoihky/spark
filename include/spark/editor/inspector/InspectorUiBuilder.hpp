#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/IUiElement.hpp"
#include "spark/ui/core/UiElementBase.hpp"

namespace Spark::Ui {
class IUiElement;
class IUiControlsFactory;
}  // namespace Spark::Ui

namespace Spark::Editor {

struct InspectorSliderBinding {
    Ui::ISlider* slider = nullptr;
};

/** Small helpers for consistent inspector rows. */
class InspectorUiBuilder final {
public:
    static void AddSectionHeader(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory, const char* title);
    static void AddMutedLabel(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory, const char* text);
    static InspectorSliderBinding& AddSlider(
            Array<InspectorSliderBinding>& bindings,
            Ui::IUiElement& parent,
            Ui::IUiControlsFactory& factory,
            const char* id,
            const char* label,
            float value,
            float minValue,
            float maxValue,
            void (*onChanged)(void* userData, float value),
            void* userData);
    static void SetSliderValue(Ui::ISlider* slider, float value) noexcept;
    static void SetVisible(Ui::IUiElement* element, bool visible) noexcept;
};

}  // namespace Spark::Editor
