#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/demo/Platformer2DDemo_detail.hpp"
#include "spark/ecs/components/SpriteAnimatorComponent.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/text/Font.hpp"

#include <algorithm>
#include <cstring>

using namespace Spark::Scripting;

namespace {

[[nodiscard]] bool TryMountUiFont(Spark::GameWorld& world) {
    if (world.GetUiFont()) {
        return true;
    }
    constexpr float kUiFontEmPx = 42.0F;
    auto uiFont = Spark::MakeShared<Spark::Font>();
    bool fontOk = uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, kUiFontEmPx);
    if (!fontOk) {
        Spark::Utf8String buildTree(SPARK_BUILD_ASSETS_DIR);
        buildTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(buildTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        Spark::Utf8String srcTree(SPARK_ASSETS_DIR);
        srcTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(srcTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        fontOk = uiFont->TryLoadTrueTypeFromFile("assets/fonts/Roboto-Regular.ttf", kUiFontEmPx);
    }
    if (!fontOk) {
        return false;
    }
    world.SetUiFont(uiFont);
    auto uiBold = Spark::MakeShared<Spark::Font>();
    bool boldOk = uiBold->TryLoadTrueTypeFromFile(SPARK_UI_BOLD_FONT_PATH, kUiFontEmPx);
    if (!boldOk) {
        Spark::Utf8String boldBuild(SPARK_BUILD_ASSETS_DIR);
        boldBuild.AppendUtf8("/fonts/Roboto-Bold.ttf");
        boldOk = uiBold->TryLoadTrueTypeFromFile(boldBuild.CStr(), kUiFontEmPx);
    }
    if (!boldOk) {
        Spark::Utf8String boldSrc(SPARK_ASSETS_DIR);
        boldSrc.AppendUtf8("/fonts/Roboto-Bold.ttf");
        boldOk = uiBold->TryLoadTrueTypeFromFile(boldSrc.CStr(), kUiFontEmPx);
    }
    if (boldOk) {
        world.SetUiBoldFont(uiBold);
    }
    return true;
}

void FillAssetsInfo(
        SparkPlatformer2DAssetsInfo* out,
        const bool kenneyTiles,
        const bool kenneyPlayer,
        const bool kenneyGem,
        const std::uint32_t playerCols) {
    if (out == nullptr) {
        return;
    }
    out->usingKenneyTilesheet = kenneyTiles ? 1 : 0;
    out->usingKenneyPlayerAtlas = kenneyPlayer ? 1 : 0;
    out->usingKenneyGem = kenneyGem ? 1 : 0;
    out->playerAtlasColumns = playerCols;
}

}  // namespace

extern "C" {

int spark_world_mount_platformer_ui_font(SparkGameWorld* world) {
    if (world == nullptr) {
        return 0;
    }
    return TryMountUiFont(*reinterpret_cast<Spark::GameWorld*>(world)) ? 1 : 0;
}

int spark_world_register_platformer2d_demo_textures(
        SparkGameWorld* world,
        SparkPlatformer2DAssetsInfo* outInfo) {
    if (world == nullptr) {
        return 0;
    }
    auto& w = *reinterpret_cast<Spark::GameWorld*>(world);

    auto platformTilesTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatTilesheet"));
    const bool platformUsingKenney = TryLoadKenneySimplifiedPlatformerTilesheet(*platformTilesTex);
    if (!platformUsingKenney) {
        *platformTilesTex = Spark::Texture2D::CreateCheckerboard(
                256,
                32,
                Spark::Vector3{0.42F, 0.36F, 0.30F},
                Spark::Vector3{0.18F, 0.52F, 0.34F});
        platformTilesTex->GetName() = Spark::Utf8String("PlatCheckerFallback");
    }
    w.RegisterTexture(platformTilesTex, "spark/plat/kenney_simplified_tilesheet");

    auto playerAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatPlayerAtlas"));
    std::uint32_t playerAtlasColumns = 5U;
    const bool playerUsingKenney = TryBuildKenneyPlayerAtlas(*playerAtlasTex, playerAtlasColumns);
    if (!playerUsingKenney) {
        *playerAtlasTex = MakePlayerRunAtlasFallback();
        playerAtlasColumns = 5U;
    }
    w.RegisterTexture(playerAtlasTex, "spark/plat/player_atlas");

    auto gemTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatGem"));
    const bool gemUsingKenney = TryLoadKenneyGemCollectible(*gemTex);
    if (!gemUsingKenney) {
        *gemTex = MakeGemTextureFallback();
        gemTex->GetName() = Spark::Utf8String("PlatGemFallback");
    }
    w.RegisterTexture(gemTex, "spark/plat/gem_collectible");

    FillAssetsInfo(outInfo, platformUsingKenney, playerUsingKenney, gemUsingKenney, playerAtlasColumns);
    return 1;
}

void spark_platformer2d_kenney_tile_uv(const std::uint32_t tileOneBased, SparkVector4* outUvRect) {
    if (outUvRect == nullptr) {
        return;
    }
    const Spark::Vector4 uv = KenneySimplifiedPlatformerTileUv(tileOneBased);
    outUvRect->x = uv.x;
    outUvRect->y = uv.y;
    outUvRect->z = uv.z;
    outUvRect->w = uv.w;
}

}  // extern "C"
