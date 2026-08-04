#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class MaterialComponent;
class Scene;
struct SceneDrawItem;

/** Copies PBR + toon fields from <c>MaterialComponent</c> into <c>item</c>. When <c>resolveTextures</c> is non-null,
 * registers normal, metallic-roughness, and emissive textures into <c>resolveTextures->sceneTextures</c> and sets
 * <c>item.normalMapLayer</c> / <c>item.metallicRoughnessMapLayer</c> / <c>item.emissiveMapLayer</c>. Base-color texture
 * stays caller-resolved. */
void ApplyMaterialComponentToSceneDrawItem(
        SceneDrawItem& item,
        const MaterialComponent* mat,
        SceneRenderParams* resolveTextures = nullptr) noexcept;

/** Splits a stable-sorted draw list into <c>params.draws</c> and <c>params.transparentDraws</c> (back-to-front). */
void PartitionSortedDrawItemsIntoSceneParams(
        const Array<SceneDrawItem>& sortedDrawList,
        SceneRenderParams& params,
        const Vector3& cameraPositionWorld) noexcept;

/**
 * Walks ECS in world, fills `outParams` (sky + lit rigid + skinned + point lights + optional particles + text).
 * Does not call `SetSceneRenderParams`; use `PaintUiCanvases` (etc.) then submit.
 */
void FillStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        float lightIntensity,
        const Vector3& ambientColor,
        bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        float sceneTimeSeconds,
        SceneRenderParams& outParams,
        SceneSpriteSortMode spriteSortMode = SceneSpriteSortMode::SortOrderOnly,
        const Scene* sceneForCulling = nullptr);

/**
 * Resolves the main camera in <c>world</c> (<c>Camera2DComponent</c> preferred when priorities tie higher)
 * and fills view/projection fields on <c>outParams</c>.
 * Returns false when no enabled camera exists (caller should supply matrices another way).
 */
[[nodiscard]] bool TryFillSceneCameraFromWorld(
        const GameWorld& world,
        float framebufferWidth,
        float framebufferHeight,
        SceneRenderParams& outParams) noexcept;

/** Fills params via `FillStandardLitSceneFromWorld` then `context.SetSceneRenderParams`. */
void SubmitStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        float lightIntensity,
        const Vector3& ambientColor,
        bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        float sceneTimeSeconds = 0.0F,
        SceneSpriteSortMode spriteSortMode = SceneSpriteSortMode::SortOrderOnly);

/**
 * Resolves the main ECS camera, fills params, and submits. Returns false when no enabled camera exists.
 */
[[nodiscard]] bool SubmitStandardLitSceneFromWorldWithCamera(
        GameWorld& world,
        IEngineContext& context,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        float lightIntensity,
        const Vector3& ambientColor,
        bool enableParticles,
        float sceneTimeSeconds = 0.0F,
        SceneSpriteSortMode spriteSortMode = SceneSpriteSortMode::SortOrderOnly);

}  // namespace Spark
