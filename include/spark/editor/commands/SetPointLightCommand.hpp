#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

namespace Editor {

struct PointLightState {
    Vector3 color{1.0F, 0.92F, 0.82F};
    float intensity = 2.4F;
    float range = 12.0F;
    bool enabled = true;
};

/** Undoable edit of <c>PointLightComponent</c> scalar fields. */
class SetPointLightCommand final : public IEditorCommand {
public:
    SetPointLightCommand(GameObject& target, PointLightState before, PointLightState after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const PointLightState& a, const PointLightState& b) noexcept;
    [[nodiscard]] static PointLightState Capture(GameObject& target);

private:
    static void Apply(GameObject& target, const PointLightState& state);

    GameObject* target = nullptr;
    PointLightState before{};
    PointLightState after{};
};

}  // namespace Editor
}  // namespace Spark
