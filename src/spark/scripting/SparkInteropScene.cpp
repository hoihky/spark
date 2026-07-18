#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/ai/GameAiSubsystem.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/GuiScene.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/scene/Texture2D.hpp"

using namespace Spark::Scripting;

namespace {

Spark::FrameTiming ToCppTiming(const SparkFrameTiming& t) {
    return {
            .deltaTimeSeconds = t.deltaTimeSeconds,
            .totalTimeSeconds = t.totalTimeSeconds,
            .frameIndex = t.frameIndex,
    };
}

Spark::SceneSpriteSortMode ToCppSortMode(const SparkSpriteSortMode mode) {
    return static_cast<Spark::SceneSpriteSortMode>(static_cast<std::uint8_t>(mode));
}

}  // namespace

extern "C" {

void spark_world_simulate_game_ai(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        SparkEngineContext* context) {
    if (world == nullptr || timing == nullptr || context == nullptr) {
        return;
    }
    Spark::SimulateGameAi(
            *reinterpret_cast<Spark::GameWorld*>(world),
            ToCppTiming(*timing),
            *reinterpret_cast<Spark::IEngineContext*>(context));
}

void spark_world_physics_simulate_2d(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        const SparkPhysicsWorld2DSettings* settings) {
    if (world == nullptr || timing == nullptr) {
        return;
    }
    Spark::PhysicsWorld2DSettings cpp{};
    if (settings != nullptr) {
        cpp.gravityY = settings->gravityY;
        cpp.maxFallSpeed = settings->maxFallSpeed;
        cpp.resolveDynamicVsDynamic = settings->resolveDynamicVsDynamic != 0;
    }
    Spark::SimulatePhysics2D(*reinterpret_cast<Spark::GameWorld*>(world), ToCppTiming(*timing), cpp);
}

void spark_world_physics_simulate_3d(SparkGameWorld* world, const SparkFrameTiming* timing) {
    if (world == nullptr || timing == nullptr) {
        return;
    }
    Spark::SimulatePhysics3D(*reinterpret_cast<Spark::GameWorld*>(world), ToCppTiming(*timing));
}

int spark_world_load_gltf(SparkGameWorld* world, const char* path) {
    if (world == nullptr || path == nullptr) {
        return 0;
    }
    const Spark::GltfAsset asset = reinterpret_cast<Spark::GameWorld*>(world)->LoadGltf(path);
    return asset.mesh ? 1 : 0;
}

int spark_world_load_skinned_gltf(SparkGameWorld* world, const char* path) {
    if (world == nullptr || path == nullptr) {
        return 0;
    }
    const Spark::SkinnedGltfAsset asset = reinterpret_cast<Spark::GameWorld*>(world)->LoadSkinnedGltf(path);
    return asset.mesh ? 1 : 0;
}

int spark_world_load_texture(SparkGameWorld* world, const char* path) {
    if (world == nullptr || path == nullptr) {
        return 0;
    }
    return reinterpret_cast<Spark::GameWorld*>(world)->LoadTexture(path) ? 1 : 0;
}

int spark_world_register_checkerboard_texture(
        SparkGameWorld* world,
        const char* cacheKey,
        const uint32_t size,
        const uint32_t tilePixels,
        const SparkVector3* colorA,
        const SparkVector3* colorB) {
    if (world == nullptr || cacheKey == nullptr || colorA == nullptr || colorB == nullptr) {
        return 0;
    }
    Spark::Texture2D tex = Spark::Texture2D::CreateCheckerboard(
            size,
            tilePixels,
            ToVector3(*colorA),
            ToVector3(*colorB));
    auto shared = Spark::MakeShared<Spark::Texture2D>(std::move(tex));
  reinterpret_cast<Spark::GameWorld*>(world)->RegisterTexture(shared, cacheKey);
    return 1;
}

void spark_scene_fill_standard_lit_from_world(
        SparkGameWorld* world,
        SparkEngineContext* context,
        const SparkMatrix4* viewProjection,
        const SparkVector3* cameraPositionWorld,
        const SparkVector3* lightDirectionWorld,
        const SparkVector3* lightColor,
        const float lightIntensity,
        const SparkVector3* ambientColor,
        const int enableParticles,
        const SparkVector3* particleCameraRight,
        const SparkVector3* particleCameraUp,
        const float sceneTimeSeconds,
        void* outParams,
        const std::uint32_t outParamsByteSize,
        const SparkSpriteSortMode spriteSortMode) {
    if (world == nullptr || context == nullptr || viewProjection == nullptr || cameraPositionWorld == nullptr ||
        lightDirectionWorld == nullptr || lightColor == nullptr || ambientColor == nullptr ||
        particleCameraRight == nullptr || particleCameraUp == nullptr || outParams == nullptr) {
        return;
    }
    if (outParamsByteSize != sizeof(Spark::SceneRenderParams)) {
        return;
    }
    auto& params = *static_cast<Spark::SceneRenderParams*>(outParams);
    Spark::FillStandardLitSceneFromWorld(
            *reinterpret_cast<Spark::GameWorld*>(world),
            *reinterpret_cast<Spark::IEngineContext*>(context),
            ToMatrix4(*viewProjection),
            ToVector3(*cameraPositionWorld),
            ToVector3(*lightDirectionWorld),
            ToVector3(*lightColor),
            lightIntensity,
            ToVector3(*ambientColor),
            enableParticles != 0,
            ToVector3(*particleCameraRight),
            ToVector3(*particleCameraUp),
            sceneTimeSeconds,
            params,
            ToCppSortMode(spriteSortMode));
}

void spark_scene_submit_standard_lit_from_world(
        SparkGameWorld* world,
        SparkEngineContext* context,
        const SparkMatrix4* viewProjection,
        const SparkVector3* cameraPositionWorld,
        const SparkVector3* lightDirectionWorld,
        const SparkVector3* lightColor,
        const float lightIntensity,
        const SparkVector3* ambientColor,
        const int enableParticles,
        const SparkVector3* particleCameraRight,
        const SparkVector3* particleCameraUp,
        const float sceneTimeSeconds,
        const SparkSpriteSortMode spriteSortMode) {
    if (world == nullptr || context == nullptr || viewProjection == nullptr || cameraPositionWorld == nullptr ||
        lightDirectionWorld == nullptr || lightColor == nullptr || ambientColor == nullptr ||
        particleCameraRight == nullptr || particleCameraUp == nullptr) {
        return;
    }
    Spark::SubmitStandardLitSceneFromWorld(
            *reinterpret_cast<Spark::GameWorld*>(world),
            *reinterpret_cast<Spark::IEngineContext*>(context),
            ToMatrix4(*viewProjection),
            ToVector3(*cameraPositionWorld),
            ToVector3(*lightDirectionWorld),
            ToVector3(*lightColor),
            lightIntensity,
            ToVector3(*ambientColor),
            enableParticles != 0,
            ToVector3(*particleCameraRight),
            ToVector3(*particleCameraUp),
            sceneTimeSeconds,
            ToCppSortMode(spriteSortMode));
}

void spark_gui_process_canvases_input(
        SparkGameWorld* world,
        SparkInput* input,
        const int framebufferWidth,
        const int framebufferHeight) {
    if (world == nullptr || input == nullptr) {
        return;
    }
    Spark::ProcessGuiCanvasesInput(
            *reinterpret_cast<Spark::GameWorld*>(world),
            *reinterpret_cast<Spark::IInput*>(input),
            framebufferWidth,
            framebufferHeight);
}

void spark_gui_paint_canvases(
        SparkGameWorld* world,
        void* params,
        const std::uint32_t paramsByteSize,
        const int framebufferWidth,
        const int framebufferHeight) {
    if (world == nullptr || params == nullptr || paramsByteSize != sizeof(Spark::SceneRenderParams)) {
        return;
    }
    Spark::PaintGuiCanvases(
            *reinterpret_cast<Spark::GameWorld*>(world),
            *static_cast<Spark::SceneRenderParams*>(params),
            framebufferWidth,
            framebufferHeight);
}

int spark_gui_consumes_game_pointer(void) {
    return Spark::GuiConsumesGamePointer() ? 1 : 0;
}

void spark_context_process_gui_input(SparkEngineContext* context) {
    if (context == nullptr) {
        return;
    }
    auto* ctx = reinterpret_cast<Spark::IEngineContext*>(context);
    Spark::Scene* scene = ctx->TryGetScene();
    if (scene == nullptr) {
        return;
    }
    int w = 0;
    int h = 0;
    ctx->GetFramebufferSize(w, h);
    float sx = 1.0F;
    float sy = 1.0F;
    ctx->GetWindow().GetContentScale(sx, sy);
    Spark::ProcessGuiCanvasesInput(*scene, ctx->GetInput(), w, h, sx, sy);
}

}  // extern "C"
