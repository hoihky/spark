#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/render/sprites2d/Scene2DRuntimeLimits.hpp"
#include "spark/scene/render/RenderTexture.hpp"

#include <cstdint>

namespace Spark {

class Texture2D;
class TilemapGridFrame;

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
/** Square ortho bounds for minimap capture (matches square RT + <c>Camera2D</c> aspect). */
struct Scene2DMinimapOrthoBounds {
    Vector3 worldCenter{0.0F, 0.0F, 0.0F};
    float worldHalfWidth = 1.0F;
    float worldHalfHeight = 1.0F;
};

[[nodiscard]] Scene2DMinimapOrthoBounds ComputeMinimapOrthoBoundsSquare(
        const TilemapGridFrame& frame,
        float padding = 1.05F) noexcept;

/** UV in [0,1] over the captured minimap image (before HUD screen Y flip). */
void WorldXYToMinimapSquareUv(
        const Vector2& worldXY,
        const Scene2DMinimapOrthoBounds& bounds,
        float& outU,
        float& outV) noexcept;

struct Scene2DCompositeViewDesc {
    Scene2DCompositeFeature feature = Scene2DCompositeFeature::Minimap;
    SharedPtr<RenderTexture> target{};
    /**
     * When set, the Vulkan backend copies the captured RT into this layer of <c>SceneRenderParams::uiTextures</c>
     * each frame (placeholder CPU pixels are overwritten on GPU).
     */
    SharedPtr<Texture2D> hudTexture{};
    /** Normalized screen rect (0–1) where UI should blit the result. */
    float screenX = 0.82F;
    float screenY = 0.02F;
    float screenW = 0.16F;
    float screenH = 0.16F;
    /** World XY center and half-extent for orthographic capture. */
    Vector3 worldCenter{0.0F, 0.0F, 0.0F};
    float worldOrthoHalfExtent = 24.0F;
    bool enabled = true;
    /** When true, skip GPU ortho capture (CPU <c>hudTexture</c> pixels only — fog mask). */
    bool useCpuHudTextureOnly = false;
};

/** Builds orthographic capture center / half-extent that frames a tilemap grid in world XY. */
void FillCompositeViewFromTilemapGridFrame(
        Scene2DCompositeViewDesc& view,
        const TilemapGridFrame& frame) noexcept;

/** Appends enabled composite descriptors from ECS into <c>params.scene2DCompositeViews</c>. */
void CollectScene2DCompositeViews(class GameWorld& world, struct SceneRenderParams& params) noexcept;

}  // namespace Spark
