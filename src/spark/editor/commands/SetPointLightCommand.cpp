#include "spark/editor/commands/SetPointLightCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

bool FloatNearlyEqual(float a, float b) noexcept {
    return std::fabs(a - b) <= Epsilon;
}

}  // namespace

SetPointLightCommand::SetPointLightCommand(
        GameObject& target,
        PointLightState before,
        PointLightState after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

void SetPointLightCommand::Apply(GameObject& target, const PointLightState& state) {
    if (PointLightComponent* light = target.GetComponent<PointLightComponent>()) {
        light->SetColor(state.color);
        light->SetIntensity(state.intensity);
        light->SetRange(state.range);
        light->SetEnabled(state.enabled);
    }
}

void SetPointLightCommand::Undo() {
    if (target != nullptr) {
        Apply(*target, before);
    }
}

void SetPointLightCommand::Redo() {
    if (target != nullptr) {
        Apply(*target, after);
    }
}

Utf8String SetPointLightCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Point Light ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("Point light edit");
}

bool SetPointLightCommand::NearlyEqual(const PointLightState& a, const PointLightState& b) noexcept {
    return FloatNearlyEqual(a.color.x, b.color.x) && FloatNearlyEqual(a.color.y, b.color.y) &&
           FloatNearlyEqual(a.color.z, b.color.z) && FloatNearlyEqual(a.intensity, b.intensity) &&
           FloatNearlyEqual(a.range, b.range) && a.enabled == b.enabled;
}

PointLightState SetPointLightCommand::Capture(GameObject& target) {
    PointLightState state{};
    if (const PointLightComponent* light = target.GetComponent<PointLightComponent>()) {
        state.color = light->GetColor();
        state.intensity = light->GetIntensity();
        state.range = light->GetRange();
        state.enabled = light->IsEnabled();
    }
    return state;
}

}  // namespace Spark::Editor
