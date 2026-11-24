#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Function.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class MaterialComponent;
class Mesh;
class MeshComponent;
class MultiMaterialComponent;
class Scene;
class SkinnedMesh;
class SkyComponent;
class Texture2D;
enum class SceneMeshSlot : std::uint8_t;
struct SceneDrawItem;
struct SceneSpriteDraw;

using FindSceneTextureFn = Function<std::int32_t(const SharedPtr<Texture2D>&, Vector2*, Vector2*)>;

std::int32_t FindOrAddSceneTexture(
        SceneRenderParams& params,
        const SharedPtr<Texture2D>& tex,
        Vector2* outUvScale = nullptr,
        Vector2* outUvOffset = nullptr,
        bool* outIsHdrLinear = nullptr);

void ApplyAlbedoTexture(
        SceneDrawItem& item,
        const SharedPtr<Texture2D>& baseColor,
        const Vector3& tint,
        const FindSceneTextureFn& findOrAddTexture);

[[nodiscard]] std::int32_t ResolveDrawableShadowFlags(
        const GameObject* object,
        SceneMeshSlot meshSlot,
        const Matrix4& worldMatrix,
        const MaterialComponent* material,
        std::int32_t defaultShadowFlags) noexcept;

[[nodiscard]] std::int32_t ResolveWaterShadowFlags(
        const MaterialComponent* material,
        const SceneRenderParams& params) noexcept;

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

[[nodiscard]] SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept;

}  // namespace Spark