#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class Scene;

/**
 * Bundles the per-frame lighting / particle / sort inputs for lit scene submit.
 * Defaults match <c>SceneRenderParams</c> and typical 3D demos.
 *
 * Prefer this over long positional argument lists; existing overloads remain valid.
 */
struct LitSceneSubmitOptions {
    Vector3 lightDirectionWorld{0.35F, 0.92F, 0.18F};
    Vector3 lightColor{1.0F, 0.97F, 0.9F};
    float lightIntensity = 0.92F;
    Vector3 ambientColor{0.05F, 0.055F, 0.07F};
    bool enableParticles = false;
    Vector3 particleCameraRight{1.0F, 0.0F, 0.0F};
    Vector3 particleCameraUp{0.0F, 1.0F, 0.0F};
    float sceneTimeSeconds = 0.0F;
    SceneSpriteSortMode spriteSortMode = SceneSpriteSortMode::SortOrderOnly;
    const Scene* sceneForCulling = nullptr;

    LitSceneSubmitOptions& WithDirectionalLight(
            const Vector3& directionWorld,
            const Vector3& color,
            const float intensity) noexcept {
        lightDirectionWorld = directionWorld;
        lightColor = color;
        lightIntensity = intensity;
        return *this;
    }

    LitSceneSubmitOptions& WithAmbient(const Vector3& color) noexcept {
        ambientColor = color;
        return *this;
    }

    LitSceneSubmitOptions& WithParticles(
            const bool enabled,
            const Vector3& cameraRight,
            const Vector3& cameraUp) noexcept {
        enableParticles = enabled;
        particleCameraRight = cameraRight;
        particleCameraUp = cameraUp;
        return *this;
    }

    LitSceneSubmitOptions& WithSceneTime(const float seconds) noexcept {
        sceneTimeSeconds = seconds;
        return *this;
    }

    LitSceneSubmitOptions& WithSpriteSortMode(const SceneSpriteSortMode mode) noexcept {
        spriteSortMode = mode;
        return *this;
    }

    LitSceneSubmitOptions& WithSceneForCulling(const Scene* scene) noexcept {
        sceneForCulling = scene;
        return *this;
    }
};

}  // namespace Spark
