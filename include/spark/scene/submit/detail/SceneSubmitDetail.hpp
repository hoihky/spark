#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Matrix4.hpp"

#include <cstdint>
#include <functional>

namespace Spark {

class GameObject;
class MaterialComponent;
class Mesh;
class MeshComponent;
class MultiMaterialComponent;
class SkinnedMesh;
class SkyComponent;
class Texture2D;
struct SceneDrawItem;

namespace SceneSubmitDetail {

std::int32_t FindOrAddSceneTexture(
        SceneRenderParams& params,
        const SharedPtr<Texture2D>& tex,
        Vector2* outUvScale = nullptr,
        Vector2* outUvOffset = nullptr,
        bool* outIsHdrLinear = nullptr);

using FindSceneTextureFn = std::function<std::int32_t(const SharedPtr<Texture2D>&, Vector2*, Vector2*)>;

void ApplyAlbedoTexture(
        SceneDrawItem& item,
        const SharedPtr<Texture2D>& baseColor,
        const Vector3& tint,
        const FindSceneTextureFn& findOrAddTexture);

void PushRigidMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const Mesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const FindSceneTextureFn& findOrAddTexture);

void PushSkinnedMeshDraws(
        Array<SceneDrawItem>& drawList,
        SceneDrawItem baseItem,
        const SkinnedMesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const FindSceneTextureFn& findOrAddTexture);
void ResolveIblEnvironmentLayer(SceneRenderParams& params) noexcept;

void PopulateSkyDrawItem(
        SceneDrawItem& item,
        const SkyComponent& sky,
        const MeshComponent& mc,
        const MaterialComponent* mat,
        const Matrix4& worldM,
        SceneRenderParams& params) noexcept;

void StableSortDrawItems(Array<SceneDrawItem>& items);
void StableSortSprites(Array<SceneSpriteDraw>& items, SceneSpriteSortMode mode);
[[nodiscard]] SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept;

}  // namespace SceneSubmitDetail

}  // namespace Spark
