#include "spark/scene/submit/LitSceneBuilder.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

LitSceneBuilder::LitSceneBuilder(GameWorld& world, IEngineContext& context) noexcept
        : world_(world), context_(context) {}

LitSceneBuilder& LitSceneBuilder::WithView(
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld) noexcept {
    viewProjection_ = viewProjection;
    cameraPositionWorld_ = cameraPositionWorld;
    hasView_ = true;
    return *this;
}

bool LitSceneBuilder::WithCameraFromWorld() noexcept {
    int fbW = 0;
    int fbH = 0;
    context_.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    SceneRenderParams cameraScratch{};
    if (!TryFillSceneCameraFromWorld(
                world_,
                static_cast<float>(fbW),
                static_cast<float>(fbH),
                cameraScratch)) {
        hasView_ = false;
        return false;
    }
    viewProjection_ = cameraScratch.viewProjection;
    cameraPositionWorld_ = cameraScratch.cameraPositionWorld;
    if (options_.enableParticles) {
        options_.particleCameraRight = cameraScratch.particleCameraRight;
        options_.particleCameraUp = cameraScratch.particleCameraUp;
    }
    hasView_ = true;
    return true;
}

LitSceneBuilder& LitSceneBuilder::WithOptions(const LitSceneSubmitOptions& options) noexcept {
    options_ = options;
    return *this;
}

LitSceneBuilder& LitSceneBuilder::WithLightingSetup(const SceneLightingSetup& setup) noexcept {
    lightingSetup_ = setup;
    hasLightingSetup_ = true;
    return *this;
}

bool LitSceneBuilder::Fill(SceneRenderParams& outParams) const noexcept {
    if (!hasView_) {
        return false;
    }
    if (hasLightingSetup_) {
        lightingSetup_.ApplyTo(outParams);
    }
    FillStandardLitSceneFromWorld(
            world_,
            context_,
            viewProjection_,
            cameraPositionWorld_,
            options_,
            outParams);
    outParams.Sanitize();
    return true;
}

bool LitSceneBuilder::Submit() const noexcept {
    SceneRenderParams params{};
    if (!Fill(params)) {
        return false;
    }
    context_.SetSceneRenderParams(params);
    return true;
}

}  // namespace Spark
