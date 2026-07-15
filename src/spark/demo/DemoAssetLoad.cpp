#include "spark/demo/DemoAssetLoad.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Spark::DemoAssets {
namespace {

[[nodiscard]] bool TryLoadFromPath(Texture2D& out, const char* path, const char* textureName) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    Texture2D tmp{};
    if (!Texture2D::TryLoadFromFile(path, tmp)) {
        return false;
    }
    if (textureName != nullptr && textureName[0] != '\0') {
        tmp.GetName() = Utf8String(textureName);
    }
    out = MoveTemp(tmp);
    return true;
}

[[nodiscard]] bool TryLoadRelativeAsset(
        Texture2D& out,
        const char* relativePath,
        const char* textureName) noexcept {
    if (relativePath == nullptr || relativePath[0] == '\0') {
        return false;
    }
    char pathBuf[768]{};
    const char* roots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};
    for (std::size_t ri = 0; roots[ri] != nullptr; ++ri) {
        std::snprintf(pathBuf, sizeof(pathBuf), "%s%s", roots[ri], relativePath);
        if (TryLoadFromPath(out, pathBuf, textureName)) {
            return true;
        }
    }
    return false;
}

void BlitTextureBottomAligned(
        const Texture2D& src,
        Array<std::uint8_t>& dstPixels,
        std::uint32_t atlasW,
        std::uint32_t atlasH,
        std::uint32_t dstX0) {
    const std::uint32_t sw = src.GetWidth();
    const std::uint32_t sh = src.GetHeight();
    const Array<std::uint8_t>& sr = src.GetRgba();
    for (std::uint32_t sy = 0; sy < sh; ++sy) {
        const std::uint32_t dy = (atlasH - sh) + sy;
        for (std::uint32_t sx = 0; sx < sw; ++sx) {
            const std::size_t si = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(sw) + sx) * 4U;
            const std::size_t di =
                    (static_cast<std::size_t>(dy) * static_cast<std::size_t>(atlasW) + static_cast<std::size_t>(dstX0) +
                     sx) *
                    4U;
            dstPixels[di] = sr[si];
            dstPixels[di + 1] = sr[si + 1];
            dstPixels[di + 2] = sr[si + 2];
            dstPixels[di + 3] = sr[si + 3];
        }
    }
}

[[nodiscard]] bool TryLoadKenneyFramesFromDir(
        const char* dirPath,
        Texture2D& idle,
        Texture2D& walkA,
        Texture2D& walkB) {
    if (dirPath == nullptr || dirPath[0] == '\0') {
        return false;
    }
    Utf8String pIdle(dirPath);
    pIdle.AppendUtf8("/platformChar_idle.png");
    Utf8String pW1(dirPath);
    pW1.AppendUtf8("/platformChar_walk1.png");
    Utf8String pW2(dirPath);
    pW2.AppendUtf8("/platformChar_walk2.png");
    if (!Texture2D::TryLoadFromFile(pIdle.CStr(), idle)) {
        return false;
    }
    if (!Texture2D::TryLoadFromFile(pW1.CStr(), walkA)) {
        return false;
    }
    if (!Texture2D::TryLoadFromFile(pW2.CStr(), walkB)) {
        return false;
    }
    return true;
}

}  // namespace

std::uint32_t KenneyPackTileNumberToSparkLinear(std::uint32_t tileOneBased) noexcept {
    if (tileOneBased == 0U) {
        tileOneBased = 1U;
    }
    const std::uint32_t cells = kKenneyTilesheetCols * kKenneyTilesheetRows;
    const std::uint32_t tl0 = (tileOneBased - 1U) % cells;
    const std::uint32_t col = tl0 % kKenneyTilesheetCols;
    const std::uint32_t rowTop = tl0 / kKenneyTilesheetCols;
    const std::uint32_t rowFromBottom = (kKenneyTilesheetRows - 1U) - rowTop;
    return col + rowFromBottom * kKenneyTilesheetCols;
}

Vector4 KenneySimplifiedPlatformerTileUv(std::uint32_t tileOneBased) noexcept {
    return SpriteAnimatorComponent::ComputeUniformGridUv(
            kKenneyTilesheetCols,
            kKenneyTilesheetRows,
            KenneyPackTileNumberToSparkLinear(tileOneBased));
}

bool TryLoadKenneySimplifiedPlatformerTilesheet(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(
            out,
            "/sprites/kenney_simplified-platformer-pack/Tilesheet/platformPack_tilesheet.png",
            "KenneySimplifiedPlatformerTilesheet");
}

bool TryBuildKenneyPlayerAtlas(Texture2D& out, std::uint32_t& outAtlasColumns) {
    const char* dirs[] = {
            SPARK_ASSETS_DIR "/sprites/kenney_simplified-platformer-pack/PNG/Characters",
            SPARK_BUILD_ASSETS_DIR "/sprites/kenney_simplified-platformer-pack/PNG/Characters",
            "assets/sprites/kenney_simplified-platformer-pack/PNG/Characters",
            SPARK_ASSETS_DIR "/sprites/kenney",
            SPARK_BUILD_ASSETS_DIR "/sprites/kenney",
            "assets/sprites/kenney",
            nullptr,
    };
    for (std::size_t di = 0; dirs[di] != nullptr; ++di) {
        Texture2D idle;
        Texture2D w1;
        Texture2D w2;
        if (!TryLoadKenneyFramesFromDir(dirs[di], idle, w1, w2)) {
            continue;
        }
        Texture2D happy{};
        Texture2D duck{};
        Utf8String pHappy(dirs[di]);
        pHappy.AppendUtf8("/platformChar_happy.png");
        Utf8String pDuck(dirs[di]);
        pDuck.AppendUtf8("/platformChar_duck.png");
        const bool haveHappy = Texture2D::TryLoadFromFile(pHappy.CStr(), happy);
        const bool haveDuck = Texture2D::TryLoadFromFile(pDuck.CStr(), duck);
        const std::uint32_t nCells = (haveHappy && haveDuck) ? 5U : 3U;

        const std::uint32_t w0 = idle.GetWidth();
        const std::uint32_t h0 = idle.GetHeight();
        const std::uint32_t wa = w1.GetWidth();
        const std::uint32_t ha = w1.GetHeight();
        const std::uint32_t wb = w2.GetWidth();
        const std::uint32_t hb = w2.GetHeight();
        std::uint32_t wh = happy.GetWidth();
        std::uint32_t hh = happy.GetHeight();
        std::uint32_t wd = duck.GetWidth();
        std::uint32_t hd = duck.GetHeight();
        if (nCells == 5U) {
            wh = (std::max)(wh, 1U);
            hh = (std::max)(hh, 1U);
            wd = (std::max)(wd, 1U);
            hd = (std::max)(hd, 1U);
        }
        const std::uint32_t cellW =
                (std::max)(w0, (std::max)(wa, (std::max)(wb, (nCells == 5U ? (std::max)(wh, wd) : wb))));
        const std::uint32_t maxH = (nCells == 5U) ? (std::max)(h0, (std::max)(ha, (std::max)(hb, (std::max)(hh, hd))))
                                                   : (std::max)(h0, (std::max)(ha, hb));
        const std::uint32_t totalW = cellW * nCells;
        if (cellW == 0 || maxH == 0) {
            continue;
        }
        Array<std::uint8_t> buf;
        buf.Resize(static_cast<std::size_t>(totalW) * static_cast<std::size_t>(maxH) * 4U);
        std::memset(buf.GetData(), 0, buf.GetSize());
        BlitTextureBottomAligned(idle, buf, totalW, maxH, 0);
        BlitTextureBottomAligned(w1, buf, totalW, maxH, cellW);
        BlitTextureBottomAligned(w2, buf, totalW, maxH, cellW * 2U);
        if (nCells == 5U) {
            BlitTextureBottomAligned(happy, buf, totalW, maxH, cellW * 3U);
            BlitTextureBottomAligned(duck, buf, totalW, maxH, cellW * 4U);
        }
        out = Texture2D(Utf8String("KenneyPlatformerPlayer"));
        out.SetPixels(totalW, maxH, MoveTemp(buf));
        outAtlasColumns = nCells;
        return true;
    }
    return false;
}

bool TryLoadKenneyGemCollectible(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(
            out,
            "/sprites/kenney_simplified-platformer-pack/PNG/Items/platformPack_item003.png",
            "KenneySimplifiedGem");
}

bool TryLoadKenneyTinyDungeonAtlas(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(
            out,
            "/sprites/kenney_tiny-dungeon/Tilemap/tilemap_packed.png",
            "KenneyTinyDungeonPacked");
}

bool TryLoadBrickTexture(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(out, "/textures/bricks.png", "KenneyBricks");
}

bool TryLoadWallBrickStoneTexture(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(out, "/textures/wall_brick_stone_center.png", "KenneyWallBrickStone");
}

bool TryLoadTerrainSoilAlbedo(Texture2D& out) noexcept {
    if (TryLoadRelativeAsset(out, "/textures/terrain/soil_albedo.png", "TerrainSoilAlbedo")) {
        return true;
    }
    return TryLoadRelativeAsset(out, "/textures/floor_ground_dirt.png", "TerrainSoilAlbedo");
}

bool TryLoadTerrainGroundAlbedo(Texture2D& out) noexcept {
    if (TryLoadRelativeAsset(out, "/textures/terrain/ground_albedo.png", "TerrainGroundAlbedo")) {
        return true;
    }
    return TryLoadRelativeAsset(out, "/textures/floor_ground_grass_overlay.png", "TerrainGroundAlbedo");
}

bool TryLoadSoilGroundTexture(Texture2D& out) noexcept {
    if (TryLoadRelativeAsset(out, "/textures/soil.png", "SoilGround")) {
        return true;
    }
    if (TryLoadTerrainSoilAlbedo(out)) {
        return true;
    }
    return TryLoadGroundDirtTexture(out);
}

bool TryLoadCharacterCameraSoilTexture(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(out, "/textures/soil.png", "CharCameraSoil");
}

bool TryLoadGroundDirtTexture(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(out, "/textures/floor_ground_dirt.png", "KenneyGroundDirt");
}

Texture2D MakeGemTextureFallback() {
    constexpr std::uint32_t kN = 24;
    Texture2D t(Utf8String("PlatGemFallback"));
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(kN) * static_cast<std::size_t>(kN) * 4U);
    for (std::uint32_t y = 0; y < kN; ++y) {
        for (std::uint32_t x = 0; x < kN; ++x) {
            const float cx = static_cast<float>(kN - 1U) * 0.5F;
            const float cy = static_cast<float>(kN - 1U) * 0.5F;
            const float dx = static_cast<float>(x) - cx;
            const float dy = static_cast<float>(y) - cy;
            const bool inside = std::fabs(dx) + std::fabs(dy) < cx * 0.92F;
            const std::uint8_t g = inside ? static_cast<std::uint8_t>(230) : static_cast<std::uint8_t>(40);
            const std::uint8_t r = inside ? static_cast<std::uint8_t>(90) : static_cast<std::uint8_t>(20);
            const std::uint8_t b = inside ? static_cast<std::uint8_t>(120) : static_cast<std::uint8_t>(30);
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kN) + x) * 4U;
            bytes[i] = r;
            bytes[i + 1] = g;
            bytes[i + 2] = b;
            bytes[i + 3] = 255;
        }
    }
    t.SetPixels(kN, kN, MoveTemp(bytes));
    return t;
}

Texture2D MakePlayerRunAtlasFallback() {
    constexpr std::uint32_t kCols = kPlayerAtlasFallbackCols;
    constexpr std::uint32_t kCellW = 40;
    constexpr std::uint32_t kCellH = 48;
    constexpr std::uint32_t kW = kCellW * kCols;
    constexpr std::uint32_t kH = kCellH;
    Texture2D t(Utf8String("PlatPlayerAtlasFallback"));
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4U);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            const std::uint32_t cell = x / kCellW;
            const float stripe = static_cast<float>(((x / 4) + (y / 6) + cell) % 5);
            Vector3 rgb{
                    0.78F + 0.06F * stripe,
                    0.32F + 0.14F * static_cast<float>(cell) / static_cast<float>(kCols - 1U),
                    0.18F + 0.04F * std::sin(static_cast<float>(x + cell * 7) * 0.11F)};
            if (cell == 3U) {
                rgb = {0.92F, 0.42F, 0.22F};
            } else if (cell == 4U) {
                rgb = {0.35F, 0.45F, 0.88F};
            }
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kW) + x) * 4U;
            bytes[i] = static_cast<std::uint8_t>(std::clamp(rgb.x * 255.0F, 0.0F, 255.0F));
            bytes[i + 1] = static_cast<std::uint8_t>(std::clamp(rgb.y * 255.0F, 0.0F, 255.0F));
            bytes[i + 2] = static_cast<std::uint8_t>(std::clamp(rgb.z * 255.0F, 0.0F, 255.0F));
            bytes[i + 3] = 255;
        }
    }
    t.SetPixels(kW, kH, MoveTemp(bytes));
    return t;
}

namespace {

[[nodiscard]] std::uint32_t HashCell(std::uint32_t seed, std::uint32_t cx, std::uint32_t cy) noexcept {
    std::uint32_t h = seed ^ cx * 0x9E3779B1u ^ cy * 0x85EBCA6Bu;
    h ^= h << 13U;
    h ^= h >> 17U;
    h ^= h << 5U;
    return h;
}

[[nodiscard]] float Hash01(std::int32_t x, std::int32_t y, std::uint32_t seed) noexcept {
    const std::uint32_t h = HashCell(seed, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0x1000000u);
}

[[nodiscard]] float SmoothNoise01(const float x, const float y, const std::uint32_t seed) noexcept {
    const std::int32_t ix = static_cast<std::int32_t>(std::floor(x));
    const std::int32_t iy = static_cast<std::int32_t>(std::floor(y));
    const float fx = x - static_cast<float>(ix);
    const float fy = y - static_cast<float>(iy);
    const float ux = fx * fx * (3.0F - 2.0F * fx);
    const float uy = fy * fy * (3.0F - 2.0F * fy);
    const float a = Hash01(ix, iy, seed);
    const float b = Hash01(ix + 1, iy, seed);
    const float c = Hash01(ix, iy + 1, seed);
    const float d = Hash01(ix + 1, iy + 1, seed);
    const float ab = a + (b - a) * ux;
    const float cd = c + (d - c) * ux;
    return ab + (cd - ab) * uy;
}

[[nodiscard]] float Smoothstep(const float edge0, const float edge1, const float x) noexcept {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] bool TryLoadSoilFloorTile(Texture2D& out, const char* relativePath) noexcept {
    return TryLoadRelativeAsset(out, relativePath, nullptr);
}

void SampleTileRepeating(const Texture2D& tile, float u, float v, float& outR, float& outG, float& outB) noexcept {
    const std::uint32_t tw = (std::max)(tile.GetWidth(), 1U);
    const std::uint32_t th = (std::max)(tile.GetHeight(), 1U);
    u = u - std::floor(u);
    v = v - std::floor(v);
    if (u < 0.0F) {
        u += 1.0F;
    }
    if (v < 0.0F) {
        v += 1.0F;
    }
    const std::uint32_t tx =
            (std::min)(static_cast<std::uint32_t>(u * static_cast<float>(tw)), tw - 1U);
    const std::uint32_t ty =
            (std::min)(static_cast<std::uint32_t>(v * static_cast<float>(th)), th - 1U);
    const Array<std::uint8_t>& px = tile.GetRgba();
    const std::size_t i = (static_cast<std::size_t>(ty) * static_cast<std::size_t>(tw) + tx) * 4U;
    outR = static_cast<float>(px[i]) / 255.0F;
    outG = static_cast<float>(px[i + 1U]) / 255.0F;
    outB = static_cast<float>(px[i + 2U]) / 255.0F;
}

[[nodiscard]] bool TryLoadPrimarySoilTile(Texture2D& out) noexcept {
    static constexpr const char* kSoilOnlyPaths[] = {
            "/textures/floor_ground_dirt.png",
            "/textures/terrain/soil_albedo.png",
            nullptr,
    };
    for (std::size_t i = 0; kSoilOnlyPaths[i] != nullptr; ++i) {
        if (TryLoadSoilFloorTile(out, kSoilOnlyPaths[i])) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool TryLoadGrassGroundTile(Texture2D& out) noexcept {
    if (TryLoadSoilFloorTile(out, "/textures/floor_ground_grass.png")) {
        return true;
    }
    return TryLoadSoilFloorTile(out, "/textures/floor_ground_grass_overlay.png");
}

[[nodiscard]] RandomSoilGroundResult BuildSmoothSoilGroundInternal(
        const std::uint32_t textureSize,
        const std::uint32_t seed,
        const bool includeGrassPatches) {
    RandomSoilGroundResult result{};
    const std::uint32_t size = (std::max)(textureSize, 64U);

    Texture2D soilTile{};
    if (!TryLoadPrimarySoilTile(soilTile)) {
        result.texture = Texture2D::CreateVariedSoilGroundPattern(size, size);
        return result;
    }

    Texture2D grassTile{};
    const bool haveGrassTile = includeGrassPatches && TryLoadGrassGroundTile(grassTile);

    static constexpr float kKenneyMicroTilePx = 64.0F;
    static constexpr float kShadeNoiseScale = 0.0105F;
    static constexpr float kGrassNoiseScale = 0.0048F;

    Array<std::uint8_t> pixels;
    pixels.Resize(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U);

    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const float tu = static_cast<float>(x) / kKenneyMicroTilePx;
            const float tv = static_cast<float>(y) / kKenneyMicroTilePx;

            float soilR = 0.0F;
            float soilG = 0.0F;
            float soilB = 0.0F;
            SampleTileRepeating(soilTile, tu, tv, soilR, soilG, soilB);

            const float shadeNoise = SmoothNoise01(
                    static_cast<float>(x) * kShadeNoiseScale,
                    static_cast<float>(y) * kShadeNoiseScale,
                    seed);
            const float shade = 0.83F + shadeNoise * 0.20F;
            float outR = soilR * shade;
            float outG = soilG * shade;
            float outB = soilB * shade;

            if (haveGrassTile) {
                float grassR = 0.0F;
                float grassG = 0.0F;
                float grassB = 0.0F;
                SampleTileRepeating(grassTile, tu, tv, grassR, grassG, grassB);

                const float grassNoiseA = SmoothNoise01(
                        static_cast<float>(x) * kGrassNoiseScale,
                        static_cast<float>(y) * kGrassNoiseScale,
                        seed ^ 0x6C8E9CF5u);
                const float grassNoiseB = SmoothNoise01(
                        static_cast<float>(x) * (kGrassNoiseScale * 1.85F) + 41.0F,
                        static_cast<float>(y) * (kGrassNoiseScale * 1.85F) + 19.0F,
                        seed ^ 0xA511E9B3u);
                const float grassMix = Smoothstep(0.50F, 0.76F, grassNoiseA * 0.62F + grassNoiseB * 0.38F);
                const float grassShade = 0.92F
                        + 0.10F
                                * SmoothNoise01(
                                        static_cast<float>(x) * 0.018F,
                                        static_cast<float>(y) * 0.018F,
                                        seed ^ 0x3C6EF372u);
                const float gr = grassR * grassShade;
                const float gg = grassG * grassShade;
                const float gb = grassB * grassShade;
                outR = outR * (1.0F - grassMix) + gr * grassMix;
                outG = outG * (1.0F - grassMix) + gg * grassMix;
                outB = outB * (1.0F - grassMix) + gb * grassMix;
            }

            const std::size_t di = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + x) * 4U;
            pixels[di] = static_cast<std::uint8_t>(std::clamp(outR * 255.0F, 0.0F, 255.0F));
            pixels[di + 1U] = static_cast<std::uint8_t>(std::clamp(outG * 255.0F, 0.0F, 255.0F));
            pixels[di + 2U] = static_cast<std::uint8_t>(std::clamp(outB * 255.0F, 0.0F, 255.0F));
            pixels[di + 3U] = 255;
        }
    }

    result.texture = Texture2D(
            Utf8String(includeGrassPatches ? "SmoothSoilGrassGround" : "SmoothSoilGround"));
    result.texture.SetPixels(size, size, MoveTemp(pixels));
    result.fromKenneyTiles = true;
    return result;
}

}  // namespace

RandomSoilGroundResult BuildJitteredSoilGroundTexture(
        const std::uint32_t textureSize,
        const std::uint32_t seed) {
    return BuildSmoothSoilGroundInternal(textureSize, seed, false);
}

RandomSoilGroundResult BuildJitteredSoilWithGrassPatchesTexture(
        const std::uint32_t textureSize,
        const std::uint32_t seed) {
    return BuildSmoothSoilGroundInternal(textureSize, seed, true);
}

bool TryLoadTerrainDemoSoilTexture(Texture2D& out) noexcept {
    return TryLoadRelativeAsset(out, "/textures/soil.png", "TerrainSoil");
}

}  // namespace Spark::DemoAssets
