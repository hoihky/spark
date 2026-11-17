#include "spark/scene/submit/SceneSubmit.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/camera/Camera.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/submit/LitSceneSubmitter.hpp"

namespace Spark {

void FillStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        const float sceneTimeSeconds,
        SceneRenderParams& outParams,
        const SceneSpriteSortMode spriteSortMode,
        const Scene* sceneForCulling) {
    LitSceneSubmitter submitter{};
    submitter.Fill(world,
            context,
            viewProjection,
            cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            enableParticles,
            particleCameraRight,
            particleCameraUp,
            sceneTimeSeconds,
            outParams,
            spriteSortMode,
            sceneForCulling);
}

void FillStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const LitSceneSubmitOptions& options,
        SceneRenderParams& outParams) {
    LitSceneSubmitter submitter{};
    submitter.Fill(world, context, viewProjection, cameraPositionWorld, options, outParams);
}

void SubmitStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        const float sceneTimeSeconds,
        const SceneSpriteSortMode spriteSortMode) {
    SceneRenderParams params{};
    FillStandardLitSceneFromWorld(world,
            context,
            viewProjection,
            cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            enableParticles,
            particleCameraRight,
            particleCameraUp,
            sceneTimeSeconds,
            params,
            spriteSortMode);
    context.SetSceneRenderParams(params);
}

void SubmitStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const LitSceneSubmitOptions& options) {
    SceneRenderParams params{};
    FillStandardLitSceneFromWorld(world, context, viewProjection, cameraPositionWorld, options, params);
    params.Sanitize();
    context.SetSceneRenderParams(params);
}

bool TryFillSceneCameraFromWorld(
        const GameWorld& world,
        const float framebufferWidth,
        const float framebufferHeight,
        SceneRenderParams& outParams) noexcept {
    SceneCameraMatrices cam{};
    if (!TryBuildSceneCameraMatrices(world, framebufferWidth, framebufferHeight, cam)) {
        return false;
    }
    outParams.viewProjection = cam.viewProjection;
    outParams.cameraPositionWorld = cam.positionWorld;
    outParams.particleCameraRight = cam.rightWorld;
    outParams.particleCameraUp = cam.upWorld;
    return true;
}

bool SubmitStandardLitSceneFromWorldWithCamera(
        GameWorld& world,
        IEngineContext& context,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const bool enableParticles,
        const float sceneTimeSeconds,
        const SceneSpriteSortMode spriteSortMode) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    SceneRenderParams params{};
    if (!TryFillSceneCameraFromWorld(
                world,
                static_cast<float>(fbW),
                static_cast<float>(fbH),
                params)) {
        return false;
    }
    FillStandardLitSceneFromWorld(
            world,
            context,
            params.viewProjection,
            params.cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            enableParticles,
            params.particleCameraRight,
            params.particleCameraUp,
            sceneTimeSeconds,
            params,
            spriteSortMode);
    context.SetSceneRenderParams(params);
    return true;
}

bool SubmitStandardLitSceneFromWorldWithCamera(
        GameWorld& world,
        IEngineContext& context,
        const LitSceneSubmitOptions& options) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    SceneRenderParams params{};
    if (!TryFillSceneCameraFromWorld(
                world,
                static_cast<float>(fbW),
                static_cast<float>(fbH),
                params)) {
        return false;
    }
    LitSceneSubmitOptions resolved = options;
    if (resolved.enableParticles) {
        resolved.particleCameraRight = params.particleCameraRight;
        resolved.particleCameraUp = params.particleCameraUp;
    }
    FillStandardLitSceneFromWorld(
            world,
            context,
            params.viewProjection,
            params.cameraPositionWorld,
            resolved,
            params);
    params.Sanitize();
    context.SetSceneRenderParams(params);
    return true;
}

}  // namespace Spark