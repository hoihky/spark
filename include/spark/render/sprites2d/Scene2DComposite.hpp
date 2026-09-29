#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/sprites2d/Scene2DRuntimeLimits.hpp"
#include "spark/scene/render/RenderTexture.hpp"

#include <cstdint>

namespace Spark {

/** Offscreen 2D views your game composites (minimap, fog mask, full-scene outline). */
enum class Scene2DCompositeFeature : std::uint8_t {
    Minimap = 0,
    FogOfWarMask = 1,
    SceneOutline = 2,
};

/**
 * Describes one RT-backed 2D composite. Fill via <c>Scene2DCompositeViewComponent</c> or game code;
 * render passes use <c>IRenderTargetService</c> + a dedicated <c>Camera2DComponent</c> ortho rig.
 */
struct Scene2DCompositeViewDesc {
    Scene2DCompositeFeature feature = Scene2DCompositeFeature::Minimap;
    SharedPtr<RenderTexture> target{};
    /** Normalized screen rect (0–1) where UI should blit the result. */
    float screenX = 0.82F;
    float screenY = 0.02F;
    float screenW = 0.16F;
    float screenH = 0.16F;
    /** World XY center and half-extent for orthographic capture. */
    Vector3 worldCenter{0.0F, 0.0F, 0.0F};
    float worldOrthoHalfExtent = 24.0F;
    bool enabled = true;
};

/** Appends enabled composite descriptors from ECS into <c>params.scene2DCompositeViews</c>. */
void CollectScene2DCompositeViews(class GameWorld& world, struct SceneRenderParams& params) noexcept;

}  // namespace Spark
