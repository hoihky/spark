#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

namespace Spark {

class GameObject;
class MaterialComponent;
class Texture2D;
struct SceneDrawItem;

namespace SceneSubmitDetail {

std::int32_t FindOrAddSceneTexture(SceneRenderParams& params, const SharedPtr<Texture2D>& tex);
void ResolveIblEnvironmentLayer(SceneRenderParams& params) noexcept;

void StableSortDrawItems(Array<SceneDrawItem>& items);
void StableSortSprites(Array<SceneSpriteDraw>& items, SceneSpriteSortMode mode);
[[nodiscard]] SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept;

}  // namespace SceneSubmitDetail

}  // namespace Spark
