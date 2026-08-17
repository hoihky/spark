#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <cstdint>
#include <functional>

namespace Spark {

class GameObject;
class MaterialComponent;
class Mesh;
class MultiMaterialComponent;
class SkinnedMesh;
class Texture2D;
struct SceneDrawItem;

namespace SceneSubmitDetail {

std::int32_t FindOrAddSceneTexture(SceneRenderParams& params, const SharedPtr<Texture2D>& tex);

void PushRigidMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const Mesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& findOrAddTexture);

void PushSkinnedMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const SkinnedMesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& findOrAddTexture);
void ResolveIblEnvironmentLayer(SceneRenderParams& params) noexcept;

void StableSortDrawItems(Array<SceneDrawItem>& items);
void StableSortSprites(Array<SceneSpriteDraw>& items, SceneSpriteSortMode mode);
[[nodiscard]] SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept;

}  // namespace SceneSubmitDetail

}  // namespace Spark
