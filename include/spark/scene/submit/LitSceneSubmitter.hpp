#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class Scene;
struct SceneRenderParams;

/**
 * Orchestrates one lit-scene collect pass: lights, meshes, water, 2D layers, particles, foliage.
 * Public entry points (<c>FillStandardLitSceneFromWorld</c>) delegate here.
 */
class LitSceneSubmitter final {
public:
    void Fill(
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
            SceneRenderParams& params,
            SceneSpriteSortMode spriteSortMode,
            const Scene* sceneForCulling);

    void Fill(
            GameWorld& world,
            IEngineContext& context,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const LitSceneSubmitOptions& options,
            SceneRenderParams& params);

private:
    void ResetSceneParams(
            GameWorld& world,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const Vector3& lightDirectionWorld,
            const Vector3& lightColor,
            float lightIntensity,
            const Vector3& ambientColor,
            float sceneTimeSeconds,
            SceneSpriteSortMode spriteSortMode,
            SceneRenderParams& params);

    void AdvanceTimeOfDay(GameWorld& world, float sceneTimeSeconds);
    void CollectPunctualLights(GameWorld& world, SceneRenderParams& params) const;

    [[nodiscard]] SceneSubmitDetail::FindSceneTextureFn MakeTextureResolver(
            GameWorld& world,
            SceneRenderParams& params) const;

    void CollectMeshDraws(
            GameWorld& world,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture,
            std::int32_t defaultShadowFlags,
            const Scene* sceneForCulling) const;

    void CollectSpritesTilemapsAndParticles(
            GameWorld& world,
            const Matrix4& viewProjection,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture,
            bool enableParticles,
            const Vector3& particleCameraRight,
            const Vector3& particleCameraUp) const;

    void CollectDecalsTextAndExtras(
            GameWorld& world,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture) const;

    void FinalizeScene(
            GameWorld& world,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture) const;
};

}  // namespace Spark