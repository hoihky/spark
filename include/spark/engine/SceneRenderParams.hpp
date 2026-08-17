#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"
#include "spark/text/Font.hpp"

#include <cstdint>

namespace Spark {

/**
 * How alpha sprite quads are ordered before the sprite pass (see <c>SceneSpriteDraw</c>).
 *
 * - <c>SortOrderOnly</c>: ascending sorting layer, then <c>sortOrder</c>.
 * - <c>SortOrderThenWorldY</c>: same keys; within equal layer + order, lower <c>sortWorldY</c> draws on top (+Y-up).
 */
enum class SceneSpriteSortMode : std::uint8_t {
    SortOrderOnly = 0,
    SortOrderThenWorldY = 1,
};

/** Transparent mesh draw ordering (see <c>SceneRenderParams::transparentDraws</c>). */
enum class SceneTransparentSortMode : std::uint8_t {
    /** Submission order only. */
    None = 0,
    /** Farther draws first (painter's algorithm, uses model translation vs camera). */
    BackToFrontByDepth = 1,
};

/** Built-in mesh slots matching the Vulkan scene vertex/index pack (see VulkanRenderer::CreateSceneGeometry). */
enum class SceneMeshSlot : std::uint8_t {
    UnitCube = 0,
    GroundPlane = 1,
    /** Arbitrary CPU mesh uploaded each frame into the renderer's dynamic scene buffers (see customMesh). */
    Custom = 2,
};

/** When set on a draw item, fragment/vertex use infinite-sky path (see scene.vert / scene.frag). */
enum class SceneSkyMode : std::uint8_t {
    None = 0,
    Box = 1,
    Dome = 2,
    Plane = 3,
};

/**
 * Point (omnidirectional) light in world space — filled from ECS PointLightComponent + transform each frame.
 * Matches the packed layout consumed by the scene fragment shader (attenuation uses range).
 */
struct ScenePointLight {
    Vector3 positionWorld{};
    float range = 8.0F;
    Vector3 color{1.0F, 1.0F, 1.0F};
    float intensity = 1.0F;
    bool castsShadow = false;
};

/**
 * Forward-rendered spotlight: position + range, axis direction (world, normalized toward lit cone center),
 * inner/outer cone as **full** cone angles in radians (outer >= inner; half-space used on GPU via cos(half)).
 */
struct SceneSpotLight {
    Vector3 positionWorld{};
    float range = 12.0F;
    Vector3 directionWorld{0.0F, 0.0F, -1.0F};
    /** Full cone opening angle in radians (smaller = tighter hotspot). */
    float innerConeRadians = 0.55F;
    /** Full cone opening angle in radians (must be >= inner). */
    float outerConeRadians = 0.85F;
    Vector3 color{1.0F, 1.0F, 1.0F};
    float intensity = 3.0F;
    bool castsShadow = false;
};

/** One billboard particle instance (CPU sim → Vulkan particle pass). */
struct SceneParticleInstance {
    Vector3 position{};
    float size = 0.1F;
    Vector4 color{1.0F, 1.0F, 1.0F, 1.0F};
};

/** One oriented decal projector instance (world pose + half-extents). */
struct SceneDecalDraw {
    Matrix4 projectorWorld = Matrix4::Identity;
    Vector3 halfExtents{0.5F, 0.5F, 0.25F};
    std::int32_t textureLayer = -1;
    float opacity = 1.0F;
};

/**
 * One alpha-blended textured quad in world space (unlit). Sorted before draw (see <c>SceneSpriteSortMode</c> on
 * <c>SceneRenderParams</c>).
 * Unit quad is XY [-0.5,0.5]² at z=0 in model space; transform scales/orients it.
 * textureLayer indexes SceneRenderParams::sceneTextures (-1 = tint-only, no texture sample).
 */
struct SceneSpriteDraw {
    Matrix4 model = Matrix4::Identity;
    Vector4 tint{1.0F, 1.0F, 1.0F, 1.0F};
    /** Atlas UV bounds (minU, minV, maxU, maxV) in normalized 0–1 space. */
    Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};
    std::int32_t textureLayer = -1;
    /** Matches sprite push constant layout (SpriteLighting2DMode). */
    SpriteLighting2DMode lightingMode = SpriteLighting2DMode::None;
    float lightingPad0 = 0.0F;
    float lightingPad1 = 0.0F;
    /** Shader-specific (see SpriteLighting2DMode). */
    Vector4 lightingParam0{1.0F, 1.0F, 1.0F, 1.0F};
    Vector4 lightingParam1{1.0F, 0.0F, 0.0F, 0.0F};
    /** Lower values draw first (behind); higher values draw on top within the sorting layer. */
    std::int32_t sortOrder = 0;
    /** Global sorting layer order from <c>RenderLayerRegistry</c> (lower draws behind). */
    std::int16_t sortingLayerOrder = 0;
    /** World Y anchor for <c>SceneSpriteSortMode::SortOrderThenWorldY</c> (sorting-group aware). */
    float sortWorldY = 0.0F;
    SceneBlendMode blendMode = kSceneBlendModeDefault;
};

/** One visible tile instance collected for a tilemap draw (grid indices + atlas tile id). */
struct SceneTilemapTileInstance {
    std::uint16_t gridX = 0;
    std::uint16_t gridY = 0;
    std::uint16_t tileId = 0;
    std::uint8_t transformFlags = 0;
    std::uint8_t tintR = 255;
    std::uint8_t tintG = 255;
    std::uint8_t tintB = 255;
    std::uint8_t tintA = 255;
    float anchorNormX = 0.5F;
    float anchorNormY = 0.5F;
};

/**
 * One ECS tilemap layer submitted to the dedicated tilemap pass. Visible tiles are stored in the shared
 * <c>SceneRenderParams::tilemapTiles</c> pool at [<c>tileBegin</c>, <c>tileBegin + tileCount</c>).
 */
struct SceneTilemapDraw {
    Matrix4 worldTransform = Matrix4::Identity;
    float tileWorldSize = 1.0F;
    std::uint32_t atlasTilesU = 1;
    std::uint32_t atlasTilesV = 1;
    float atlasMarginPixels = 0.0F;
    float atlasSpacingPixels = 0.0F;
    std::uint32_t atlasTextureWidth = 1;
    std::uint32_t atlasTextureHeight = 1;
    float atlasLayerUvScaleU = 1.0F;
    float atlasLayerUvScaleV = 1.0F;
    std::uint32_t atlasTilePixelWidth = 0;
    std::uint32_t atlasTilePixelHeight = 0;
    std::int32_t textureLayer = -1;
    std::int32_t sortOrderBase = 0;
    std::int16_t sortingLayerOrder = 0;
    SceneBlendMode blendMode = kSceneBlendModeDefault;
    std::uint32_t tileBegin = 0;
    std::uint32_t tileCount = 0;
    TilemapLayerSortMode instanceSortMode = TilemapLayerSortMode::GridOrder;
    /** Tie-break vs sprites at the same sorting layer + order (see <c>SceneSpriteSortMode</c>). */
    float sortWorldYAnchor = 0.0F;
};

struct SceneDrawItem {
    SceneMeshSlot mesh = SceneMeshSlot::UnitCube;
    SceneSkyMode skyMode = SceneSkyMode::None;
    Matrix4 model = Matrix4::Identity;
    Vector3 albedo{1.0F, 1.0F, 1.0F};
    /** Index into SceneRenderParams::sceneTextures; -1 = no texture (albedo only). */
    std::int32_t textureLayer = -1;
    /** Tangent-space normal map layer (-1 = use vertex/interpolated normal only). */
    std::int32_t normalMapLayer = -1;
    /**
     * Occlusion (R), roughness (G), metallic (B) in one texture (glTF metallic-roughness convention).
     * -1 = use scalar metallic/roughness only; ambient occlusion from R multiplies hemisphere ambient when set.
     */
    std::int32_t metallicRoughnessMapLayer = -1;
    /** Emissive map layer in <c>sceneTextures</c> (-1 = uniform emissive only, no texture). */
    std::int32_t emissiveMapLayer = -1;
    /** Required when mesh == Custom (rigid); used by VulkanRenderer to pack GPU geometry. */
    SharedPtr<Mesh> customMesh{};
    /** When set, custom mesh path uses skinned vertices + jointPalette (mutually exclusive with customMesh). */
    SharedPtr<SkinnedMesh> skinnedMesh{};
    /** Index into <c>Mesh::GetSubmeshes()</c> / <c>SkinnedMesh::GetSubmeshes()</c>; <c>kSceneDrawFullSubmesh</c> = entire buffer. */
    std::uint32_t submeshIndex = kSceneDrawFullSubmesh;
    /** Joint matrices for skinning: skinMatrix[j] = worldJoint[j] * invBind[j]; size == skeleton joint count. */
    Array<Matrix4> jointPalette{};
    /** Physically based material parameters (GGX + Fresnel in shader). */
    float metallic = 0.0F;
    float roughness = 0.45F;
    /** glTF metallicFactor / roughnessFactor / occlusionStrength (default 1). */
    float metallicFactor = 1.0F;
    float roughnessFactor = 1.0F;
    float occlusionStrength = 1.0F;
    /**
     * Shadow participation: <c>kSceneShadowCast</c> / <c>kSceneShadowReceive</c>
     * (<c>SceneLightingProfile.hpp</c>). Default = cast + receive.
     */
    std::int32_t shadowFlags = kSceneShadowCastAndReceive;
    Vector3 emissiveColor{};
    float emissiveIntensity = 0.0F;
    /** glTF emissiveFactor (RGB multiplier on emissive color / map). */
    Vector3 emissiveFactor{Vector3::One};
    /** When <c>ToonCel</c>, fragment shader uses stepped diffuse + rim (see <c>toonDiffuseBands</c> / rim fields). */
    SceneShadingModel shadingModel = SceneShadingModel::LitPbr;
    /** Quantized sun / point-light diffuse steps (clamped 2–8 on GPU). */
    std::int32_t toonDiffuseBands = 3;
    float toonRimIntensity = 0.35F;
    float toonRimPower = 4.0F;
    /** When true, both faces are rasterized (glTF doubleSided / foliage). */
    bool doubleSided = false;
    /** 1 = opaque; &lt; 1 submits to <c>transparentDraws</c> (back-to-front sorted). */
    float opacity = 1.0F;
};

/** Gradient mode for UI rects (per-corner colors are interpolated in ui_solid.frag). */
enum class ScreenRectGradient : std::uint8_t {
    None = 0,
    /** Top edge = color, bottom edge = colorB. */
    Vertical = 1,
    /** Left edge = color, right edge = colorB. */
    Horizontal = 2,
};

/** Filled axis-aligned rectangle in framebuffer pixels (Y downward). */
struct ScreenRectDraw {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    Vector3 color{1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;
    /** Second stop when gradient != None; ignored for solid fills. */
    Vector3 colorB{1.0F, 1.0F, 1.0F};
    ScreenRectGradient gradient = ScreenRectGradient::None;
    /**
     * When true, Vulkan scissor is set to the clip rect (intersected with the framebuffer) for this draw
     * and any consecutive draws sharing the same clip in a batch.
     */
    bool clipEnabled = false;
    float clipX = 0.0F;
    float clipY = 0.0F;
    float clipW = 0.0F;
    float clipH = 0.0F;
    /** Monotonic key so rects and texts can be merged for draw order (see VulkanRenderer UI pass). */
    std::uint32_t paintOrder = 0;
    SceneBlendMode blendMode = kSceneBlendModeDefault;
};

/** Textured quad in framebuffer pixels (Y downward). Rendered in the UI pass between solids and text. */
struct ScreenSpriteDraw {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    /** (minU, minV, maxU, maxV) */
    Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};
    std::int32_t textureLayer = -1;
    Vector3 tint{1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;
    bool clipEnabled = false;
    float clipX = 0.0F;
    float clipY = 0.0F;
    float clipW = 0.0F;
    float clipH = 0.0F;
    std::uint32_t paintOrder = 0;
    SceneBlendMode blendMode = kSceneBlendModeDefault;
};

/** One line of UI text in framebuffer pixels (Y downward). Uses SceneRenderParams::uiFont when drawing. */
struct ScreenTextDraw {
    Utf8String text{};
    float x = 0.0F;
    float y = 0.0F;
    float sizePixels = 16.0F;
    Vector3 color{1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;
    /** Faux bold: extra glyph passes with sub-pixel horizontal offsets (regular TTF only). */
    bool bold = false;
    bool clipEnabled = false;
    float clipX = 0.0F;
    float clipY = 0.0F;
    float clipW = 0.0F;
    float clipH = 0.0F;
    /** Monotonic key merged with <c>ScreenRectDraw::paintOrder</c> for correct rect/text interleaving. */
    std::uint32_t paintOrder = 0;
    SceneBlendMode blendMode = kSceneBlendModeDefault;
};

/**
 * Per-frame 3D scene data consumed by VulkanRenderer (optional on other IFramePresenter implementations).
 * For 2D, build viewProjection with Matrix4::OrthographicVulkan and a view matrix, or use Camera2D::ViewProjection
 * (world +Y up; framebuffer Y still increases downward for screenRects / screenTexts).
 */
struct SceneRenderParams {
    static constexpr std::uint32_t MaxSceneTextures = 32;
    /** Must match GPU clustered lights SSBO capacity. */
    static constexpr std::uint32_t MaxPointLights = 256;

    Matrix4 viewProjection{};
    Array<SceneDrawItem> draws;
    /** Alpha-blended lit meshes (after opaque <c>draws</c>, before sprites). */
    Array<SceneDrawItem> transparentDraws;
    /** Textures packed into a GPU 2D array (same order as textureLayer indices in draws). Max 16. */
    Array<SharedPtr<Texture2D>> sceneTextures;
    /** Normalized; direction from a surface point toward the light (N·L). */
    Vector3 lightDirectionWorld{0.35F, 0.92F, 0.18F};
    Vector3 cameraPositionWorld{};
    Vector3 lightColor{1.0F, 0.97F, 0.9F};
    float lightIntensity = 0.92F;
    Vector3 ambientColor{0.05F, 0.055F, 0.07F};
    /**
     * When true, the renderer builds a directional shadow map from lightDirectionWorld and modulates sun BRDF.
     * When false, sun lighting is unshadowed (cheaper; useful for 2D-style ortho scenes).
     */
    bool directionalShadowsEnabled = true;
    /** When true, point/spot lights with <c>castsShadow</c> render shadow maps (capped per frame). */
    bool punctualShadowsEnabled = true;

    /** Screen-space ambient occlusion applied before tonemap (requires retained scene depth). */
    bool ssaoEnabled = true;
    float ssaoRadius = 0.35F;
    float ssaoBias = 0.02F;
    float ssaoStrength = 0.65F;
    /** Depth bias for shadow map compare (tune if you see acne). */
    float shadowBias = 0.0026F;
    /**
     * Extra shadow compare bias on grazing angles (meters-scale coefficient in scene.frag): adds
     * `shadowNormalBias * 0.22 * (1 - N·L)` to the depth bias. Does **not** move the world position before
     * `worldToShadowClip` (that broke UV vs depth consistency). Tune with `shadowBias` for acne vs Peter-panning.
     */
    float shadowNormalBias = 0.048F;
    /**
     * When true, scene.frag flips shadow-map V when sampling (`uv.y = 1 - uv.y`). Packed into GPU `viewportSize.w`
     * (xy remain framebuffer size). Enable on stacks where depth-compare sampling row order does not match NDC→UV
     * (often MoltenVK / Metal); if shadows look vertically mirrored or missing on the ground, toggle this.
     */
    bool shadowDepthSampleFlipV = false;

    /**
     * Shared lighting preset: cascades, exposure, shadow fade, hemisphere/probe, time-of-day defaults.
     * Explicit fields below override preset values when &gt; 0 (see <c>ResolveSceneLightingFromParams</c>).
     */
    SceneLightingProfile lightingProfile = SceneLightingProfile::Default;
    /** Tonemap exposure multiplier; 0 = use preset from <c>lightingProfile</c>. */
    float exposure = 0.0F;
    /** CSM near plane (world-space split base); 0 = preset. */
    float shadowCascadeNear = 0.0F;
    /** CSM far plane; 0 = preset. */
    float shadowCascadeFar = 0.0F;
    /**
     * World-space distance from camera where directional shadows fade to unshadowed.
     * 0 = use preset (outdoor ~150 m, interior ~45 m, default = no fade).
     */
    float shadowDistanceMax = 0.0F;
    /**
     * Shadow fade begins at <c>shadowDistanceMax * shadowFadeStartRatio</c>; 0 = preset (typically ~0.82).
     */
    float shadowFadeStartRatio = 0.0F;
    /** Multiplier on resolved hemisphere ground color; 0 = use preset ambient scale only. */
    float ambientScale = 0.0F;
    /** Default cast/receive for draws; per-draw <c>shadowFlags</c> can still disable. */
    bool shadowsCastByDefault = true;
    bool shadowsReceiveByDefault = true;
    /**
     * Analytic sun + hemisphere from <c>timeOfDay</c>. Outdoor/NightInterior presets enable by default
     * unless you set <c>useTimeOfDay = false</c>.
     */
    bool useTimeOfDay = false;
    /** 0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset. */
    float timeOfDay = 0.5F;

    /** Regional fog from <c>FogVolumeComponent</c> (data path; GPU may ignore until wired). */
    bool fogEnabled = false;
    Vector3 fogColor{0.72F, 0.78F, 0.86F};
    float fogDensity = 0.0F;
    float fogStart = 8.0F;
    float fogEnd = 96.0F;

    /** Dynamic point lights (typically from PointLightComponent); only the first MaxPointLights are sent to GPU. */
    Array<ScenePointLight> pointLights;

    static constexpr std::uint32_t MaxSpotLights = 128;
    /** Cone lights (SpotLightComponent); first MaxSpotLights only on GPU. */
    Array<SceneSpotLight> spotLights;

    /**
     * Image-based lighting for PBR metals (equirect in sceneTextures). -1 = auto-detect from sky draw
     * with a bound texture, else procedural hemisphere from ambient colors.
     */
    std::int32_t iblEnvironmentLayer = -1;
    /** Multiplier on diffuse + specular IBL contribution. */
    float iblIntensity = 1.0F;
    bool iblEnabled = true;

    /** Authoritative scene time (seconds) for animated sprite lighting; set by SubmitStandardLitSceneFromWorld. */
    float sceneTimeSeconds = 0.0F;

    /**
     * Sprite pass ordering. <c>SortOrderThenWorldY</c> uses each draw’s model translation world Y (<c>m[13]</c>) as a
     * secondary key after <c>SceneSpriteDraw::sortOrder</c> (same value on all sprites is typical for ARPG layers).
     */
    SceneSpriteSortMode spriteSortMode = SceneSpriteSortMode::SortOrderOnly;
    SceneTransparentSortMode transparentSortMode = SceneTransparentSortMode::BackToFrontByDepth;

    /** Alpha-blended sprite quads (after tilemaps, before additive particles). Sorted per <c>spriteSortMode</c>. */
    static constexpr std::uint32_t MaxSprites = 8192;
    Array<SceneSpriteDraw> sprites;

    /** Dedicated tilemap pass: culled tile instances + layer metadata (before sprites). */
    static constexpr std::uint32_t MaxTilemapDraws = 64;
    static constexpr std::uint32_t MaxTilemapTiles = 65536;
    Array<SceneTilemapDraw> tilemaps;
    Array<SceneTilemapTileInstance> tilemapTiles;

    /** Billboard particles (additive pass). Filled from ParticleEmitterComponent. */
    static constexpr std::uint32_t MaxParticles = 8192;
    Array<SceneParticleInstance> particles;
    /** Camera-facing billboard axes in world space (normalized). */
    Vector3 particleCameraRight{1.0F, 0.0F, 0.0F};
    Vector3 particleCameraUp{0.0F, 1.0F, 0.0F};

    /** Oriented decal volumes collected from <c>DecalProjectorComponent</c> (renderer may ignore until implemented). */
    static constexpr std::uint32_t MaxDecals = 256;
    Array<SceneDecalDraw> decals;

    /** Optional UI font (TrueType atlas); when null, screen text draws are skipped. */
    SharedPtr<Font> uiFont{};
    /** Optional bold UI font (same bake size as uiFont); GPU atlas layer 1. When null, bold uses faux stroke. */
    SharedPtr<Font> uiBoldFont{};
    /** Solid quads (panels, control chrome). */
    Array<ScreenRectDraw> screenRects;
    /** Textured UI quads (skin sprites, icons). Drawn after rects, before text, per layer. */
    static constexpr std::uint32_t MaxUiTextures = 16;
    Array<SharedPtr<Texture2D>> uiTextures;
    Array<ScreenSpriteDraw> screenSprites;
    /** Screen-space labels (e.g. from <c>GuiPaintContext::DrawText</c>). */
    Array<ScreenTextDraw> screenTexts;
    /**
     * Optional layer after all main <c>screenRects</c> and <c>screenTexts</c> (solid pass then text pass).
     */
    Array<ScreenRectDraw> screenOverlayRects;
    Array<ScreenSpriteDraw> screenOverlaySprites;
    Array<ScreenTextDraw> screenOverlayTexts;
    /**
     * Final UI layer after overlay (solid pass then text in <c>VulkanRenderer::RecordScreenUi</c>). Used for
     * e.g. dropdown lists so they composite on top without incorrect blending against semi-transparent parents.
     */
    Array<ScreenRectDraw> screenLateRects;
    Array<ScreenSpriteDraw> screenLateSprites;
    Array<ScreenTextDraw> screenLateTexts;
    /** Increments for each emitted <c>ScreenRectDraw</c> / <c>ScreenTextDraw</c>; reset when clearing UI lists. */
    std::uint32_t uiPaintOrderNext = 0;

    /**
     * When set, the 3D scene pass is scissored to this framebuffer rectangle (pixels, Y down).
     * Used by the scene editor so the ground plane is not drawn under the left inspector strip.
     */
    bool worldViewportScissorEnabled = false;
    float worldViewportScissorX = 0.0F;
    float worldViewportScissorY = 0.0F;
    float worldViewportScissorW = 0.0F;
    float worldViewportScissorH = 0.0F;

    [[nodiscard]] std::uint32_t NextUiPaintOrder() noexcept { return ++uiPaintOrderNext; }
};

}  // namespace Spark
