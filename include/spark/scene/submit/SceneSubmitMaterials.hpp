#pragma once

#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

class MaterialComponent;
struct SceneDrawItem;

/** Copies PBR + toon fields from <c>MaterialComponent</c> into <c>item</c>. */
void ApplyMaterialComponentToSceneDrawItem(
        SceneDrawItem& item,
        const MaterialComponent* mat,
        SceneRenderParams* resolveTextures = nullptr) noexcept;

void ApplyMultiMaterialSlotToSceneDrawItem(
        SceneDrawItem& item,
        const MultiMaterialComponent::Slot& slot,
        SceneRenderParams* resolveTextures = nullptr) noexcept;

}  // namespace Spark