#include "spark/editor/inspector/InspectorUiBuilder.hpp"

#include "spark/ui/factory/IUiControlsFactory.hpp"
#include "spark/ui/spark/UiChild.hpp"

namespace Spark::Editor {

namespace {

void OnSliderChanged(void* userData, const float value) {
    auto* binding = static_cast<InspectorSliderBinding*>(userData);
    if (binding != nullptr && binding->apply != nullptr) {
        binding->apply(binding->userData, value);
    }
}

}  // namespace

void InspectorUiBuilder::AddSectionHeader(
        Ui::IUiElement& parent,
        Ui::IUiControlsFactory& factory,
        const char* const title) {
    Ui::LabelDesc desc{};
    desc.id = Utf8String(title);
    desc.text = Utf8String(title);
    auto label = factory.CreateLabel(desc);
    AdoptUiChild(parent, MoveTemp(label));
}

void InspectorUiBuilder::AddMutedLabel(
        Ui::IUiElement& parent,
        Ui::IUiControlsFactory& factory,
        const char* const text) {
    Ui::LabelDesc desc{};
    desc.id = Utf8String(text);
    desc.text = Utf8String(text);
    desc.muted = true;
    auto label = factory.CreateLabel(desc);
    AdoptUiChild(parent, MoveTemp(label));
}

InspectorSliderBinding& InspectorUiBuilder::AddSlider(
        Array<InspectorSliderBinding>& bindings,
        Ui::IUiElement& parent,
        Ui::IUiControlsFactory& factory,
        const char* const id,
        const char* const label,
        const float value,
        const float minValue,
        const float maxValue,
        void (*const onChanged)(void* userData, float value),
        void* const userData) {
    bindings.PushBack({});
    InspectorSliderBinding& binding = bindings[bindings.GetSize() - 1U];
    binding.apply = onChanged;
    binding.userData = userData;

    Ui::SliderDesc desc{};
    desc.id = Utf8String(id);
    desc.label = Utf8String(label);
    desc.value = value;
    desc.minValue = minValue;
    desc.maxValue = maxValue;
    desc.dragInput = true;
    auto sliderUp = factory.CreateSlider(desc);
    binding.slider = sliderUp.Get();

    Ui::UiFloatCallback cb{};
    cb.fn = OnSliderChanged;
    cb.userData = &binding;
    binding.slider->SetOnChanged(cb);

    AdoptUiChild(parent, MoveTemp(sliderUp));
    return binding;
}

void InspectorUiBuilder::SetSliderValue(Ui::ISlider* const slider, const float value) noexcept {
    if (slider != nullptr) {
        slider->SetValue(value);
    }
}

void InspectorUiBuilder::SetVisible(Ui::IUiElement* const element, const bool visible) noexcept {
    if (auto* base = dynamic_cast<Ui::UiElementBase*>(element)) {
        base->SetVisible(visible);
    }
}

}  // namespace Spark::Editor
