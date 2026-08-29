#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class Texture2D;

namespace Editor {

struct MaterialInspectorState {
    Vector3 tint{Vector3::One};
    float metallic = 0.0F;
    float roughness = 0.45F;
    Vector3 emissiveColor{};
    float emissiveIntensity = 0.0F;
    Utf8String baseColorTexturePath{};
    Utf8String normalTexturePath{};
    Utf8String metallicRoughnessTexturePath{};
    Utf8String emissiveTexturePath{};
};

/** Undoable edit of common <c>MaterialComponent</c> scalar fields shown in the inspector. */
class SetMaterialInspectorCommand final : public IEditorCommand {
public:
    SetMaterialInspectorCommand(GameObject& target, MaterialInspectorState before, MaterialInspectorState after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const MaterialInspectorState& a, const MaterialInspectorState& b) noexcept;
    [[nodiscard]] static MaterialInspectorState Capture(GameObject& target);
    static void ApplyState(GameObject& target, const MaterialInspectorState& state);

private:
    static void Apply(GameObject& target, GameWorld& world, const MaterialInspectorState& state);
    static SharedPtr<Texture2D> ResolveTexture(GameWorld& world, const Utf8String& path);

    GameObject* target = nullptr;
    MaterialInspectorState before{};
    MaterialInspectorState after{};
};

}  // namespace Editor
}  // namespace Spark
