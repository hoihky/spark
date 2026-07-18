#pragma once

#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>

namespace Spark::DemoAssets {

constexpr std::uint32_t kKenneyTilesheetCols = 14U;
constexpr std::uint32_t kKenneyTilesheetRows = 7U;
constexpr std::uint32_t kPlayerAtlasFallbackCols = 5U;

/** Kenney floor PNGs are 64×64; repeat every ~2 world units on large XZ surfaces. */
constexpr float kKenneyTileWorldUnitsPerRepeat = 2.0F;

/** One texture span across a mesh whose XZ half-extent is @p halfExtent (procedural 0–1 UVs). */
[[nodiscard]] constexpr float ProceduralTextureSpanWorldUnits(float halfExtent) noexcept {
    return (std::max)(halfExtent * 2.0F, 1.0e-3F);
}

[[nodiscard]] std::uint32_t KenneyPackTileNumberToSparkLinear(std::uint32_t tileOneBased) noexcept;

[[nodiscard]] Vector4 KenneySimplifiedPlatformerTileUv(std::uint32_t tileOneBased) noexcept;

[[nodiscard]] bool TryLoadKenneySimplifiedPlatformerTilesheet(Texture2D& out) noexcept;

[[nodiscard]] bool TryBuildKenneyPlayerAtlas(Texture2D& out, std::uint32_t& outAtlasColumns);

[[nodiscard]] bool TryLoadKenneyGemCollectible(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadKenneyTinyDungeonAtlas(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadBrickTexture(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadWallBrickStoneTexture(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadTerrainSoilAlbedo(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadTerrainGroundAlbedo(Texture2D& out) noexcept;

/** Ground / soil diffuse for character and outdoor demos (Kenney dirt when dedicated soil PNG is absent). */
[[nodiscard]] bool TryLoadSoilGroundTexture(Texture2D& out) noexcept;

/** Character Camera demo ground — `assets/textures/soil.png` only. */
[[nodiscard]] bool TryLoadCharacterCameraSoilTexture(Texture2D& out) noexcept;

/** Terrain demo ground — `assets/textures/soil.png` only. */
[[nodiscard]] bool TryLoadTerrainDemoSoilTexture(Texture2D& out) noexcept;

[[nodiscard]] bool TryLoadGroundDirtTexture(Texture2D& out) noexcept;

/**
 * Smooth soil ground texture (Kenney dirt micro-tiled with low-frequency brightness noise).
 * Map once across the mesh via ProceduralTextureSpanWorldUnits.
 */
struct RandomSoilGroundResult {
    Texture2D texture{};
    /** True when built from Kenney soil PNGs (still use single-span world UVs). */
    bool fromKenneyTiles = false;
};

/** Soil-only, smooth brightness variation (terrain). */
[[nodiscard]] RandomSoilGroundResult BuildJitteredSoilGroundTexture(
        std::uint32_t textureSize = 1024U,
        std::uint32_t seed = 0x7E2A11C0u);

/** Soil base with smooth grass patches blended in (character ground). */
[[nodiscard]] RandomSoilGroundResult BuildJitteredSoilWithGrassPatchesTexture(
        std::uint32_t textureSize = 768U,
        std::uint32_t seed = 0xC41A70E1u);

[[nodiscard]] Texture2D MakeGemTextureFallback();

[[nodiscard]] Texture2D MakePlayerRunAtlasFallback();

/** Kenney slime (2-frame) when `PNG/Enemies` is present; else Tiny Dungeon ghost; else procedural. */
struct PlatformerEnemyAtlasResult {
    Texture2D texture{};
    std::uint32_t columns = 1U;
    bool fromKenneySlime = false;
    bool fromTinyDungeon = false;
};

[[nodiscard]] PlatformerEnemyAtlasResult BuildPlatformerEnemyAtlas();

/** Small additive projectile sprite (used when no dedicated PNG is bundled). */
[[nodiscard]] Texture2D MakeEnemyBulletTextureFallback();

}  // namespace Spark::DemoAssets
