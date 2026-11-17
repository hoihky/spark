#include "spark/scene/submit/LitScene.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

LitScene::Builder::Builder(GameWorld& world, IEngineContext& context) noexcept
        : hostWorld(world), engineContext(context) {}

LitScene::Builder& LitScene::Builder::WithView(
        const Matrix4& view,
        const Vector3& cameraWorld) noexcept {
    viewProjection = view;
    cameraPositionWorld = cameraWorld;
    hasView = true;
    return *this;
}

bool LitScene::Builder::WithCameraFromWorld() noexcept {
    int fbW = 0;
    int fbH = 0;
    engineContext.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    SceneRenderParams cameraScratch{};
    if (!TryFillSceneCameraFromWorld(
                hostWorld,
                static_cast<float>(fbW),
                static_cast<float>(fbH),
                cameraScratch)) {
        hasView = false;
        return false;
    }
    viewProjection = cameraScratch.viewProjection;
    cameraPositionWorld = cameraScratch.cameraPositionWorld;
    if (submitOptions.enableParticles) {
        submitOptions.particleCameraRight = cameraScratch.particleCameraRight;
        submitOptions.particleCameraUp = cameraScratch.particleCameraUp;
    }
    hasView = true;
    return true;
}

LitScene::Builder& LitScene::Builder::WithOptions(const LitSceneSubmitOptions& options) noexcept {
    submitOptions = options;
    return *this;
}

LitScene::Builder& LitScene::Builder::WithLightingSetup(const SceneLightingSetup& setup) noexcept {
    lightingSetup = setup;
    hasLightingSetup = true;
    return *this;
}

bool LitScene::Builder::Fill(SceneRenderParams& outParams) const noexcept {
    if (!hasView) {
        return false;
    }
    if (hasLightingSetup) {
        lightingSetup.ApplyTo(outParams);
    }
    FillStandardLitSceneFromWorld(
            hostWorld,
            engineContext,
            viewProjection,
            cameraPositionWorld,
            submitOptions,
            outParams);
    outParams.Sanitize();
    return true;
}

bool LitScene::Builder::Submit() const noexcept {
    SceneRenderParams params{};
    if (!Fill(params)) {
        return false;
    }
    engineContext.SetSceneRenderParams(params);
    return true;
}

}  // namespace Spark