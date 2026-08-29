#include "spark/editor/commands/SetMaterialInspectorCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/math/Constants.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

bool FloatNearlyEqual(float a, float b) noexcept {
    return std::fabs(a - b) <= Epsilon;
}

bool Vec3NearlyEqual(const Vector3& a, const Vector3& b) noexcept {
    return FloatNearlyEqual(a.x, b.x) && FloatNearlyEqual(a.y, b.y) && FloatNearlyEqual(a.z, b.z);
}

Utf8String TexturePathFromComponent(const SharedPtr<Texture2D>& texture) {
    if (!texture) {
        return {};
    }
    return texture->GetName();
}

}  // namespace

SetMaterialInspectorCommand::SetMaterialInspectorCommand(
        GameObject& target,
        MaterialInspectorState before,
        MaterialInspectorState after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

SharedPtr<Texture2D> SetMaterialInspectorCommand::ResolveTexture(GameWorld& world, const Utf8String& path) {
    if (path.IsEmpty()) {
        return {};
    }
    if (SharedPtr<Texture2D> cached = world.TryGetTextureByKeyOrPath(path.CStr())) {
        return cached;
    }
    return world.LoadTexture(path.CStr());
}

void SetMaterialInspectorCommand::Apply(
        GameObject& target,
        GameWorld& world,
        const MaterialInspectorState& state) {
    if (MaterialComponent* material = target.GetComponent<MaterialComponent>()) {
        material->SetTint(state.tint);
        material->SetMetallic(state.metallic);
        material->SetRoughness(state.roughness);
        material->SetEmissive(state.emissiveColor, state.emissiveIntensity);
        material->SetBaseColorTexture(ResolveTexture(world, state.baseColorTexturePath));
        material->SetNormalTexture(ResolveTexture(world, state.normalTexturePath));
        material->SetMetallicRoughnessTexture(ResolveTexture(world, state.metallicRoughnessTexturePath));
        material->SetEmissiveTexture(ResolveTexture(world, state.emissiveTexturePath));
    }
}

void SetMaterialInspectorCommand::ApplyState(GameObject& target, const MaterialInspectorState& state) {
    Apply(target, target.GetWorld(), state);
}

void SetMaterialInspectorCommand::Undo() {
    if (target != nullptr) {
        Apply(*target, target->GetWorld(), before);
    }
}

void SetMaterialInspectorCommand::Redo() {
    if (target != nullptr) {
        Apply(*target, target->GetWorld(), after);
    }
}

Utf8String SetMaterialInspectorCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Material ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("Material edit");
}

bool SetMaterialInspectorCommand::NearlyEqual(
        const MaterialInspectorState& a,
        const MaterialInspectorState& b) noexcept {
    return Vec3NearlyEqual(a.tint, b.tint) && FloatNearlyEqual(a.metallic, b.metallic) &&
           FloatNearlyEqual(a.roughness, b.roughness) && Vec3NearlyEqual(a.emissiveColor, b.emissiveColor) &&
           FloatNearlyEqual(a.emissiveIntensity, b.emissiveIntensity) && a.baseColorTexturePath == b.baseColorTexturePath &&
           a.normalTexturePath == b.normalTexturePath &&
           a.metallicRoughnessTexturePath == b.metallicRoughnessTexturePath &&
           a.emissiveTexturePath == b.emissiveTexturePath;
}

MaterialInspectorState SetMaterialInspectorCommand::Capture(GameObject& target) {
    MaterialInspectorState state{};
    if (const MaterialComponent* material = target.GetComponent<MaterialComponent>()) {
        state.tint = material->GetTint();
        state.metallic = material->GetMetallic();
        state.roughness = material->GetRoughness();
        state.emissiveColor = material->GetEmissiveColor();
        state.emissiveIntensity = material->GetEmissiveIntensity();
        state.baseColorTexturePath = TexturePathFromComponent(material->GetBaseColorTexture());
        state.normalTexturePath = TexturePathFromComponent(material->GetNormalTexture());
        state.metallicRoughnessTexturePath = TexturePathFromComponent(material->GetMetallicRoughnessTexture());
        state.emissiveTexturePath = TexturePathFromComponent(material->GetEmissiveTexture());
    }
    return state;
}

}  // namespace Spark::Editor
