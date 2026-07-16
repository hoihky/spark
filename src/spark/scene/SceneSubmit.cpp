#include "spark/scene/SceneSubmit.hpp"

#include "spark/scene/detail/SceneSubmitDetail.hpp"
#include "spark/scene/detail/SceneSubmitLighting.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/Camera.hpp"
#include "spark/ecs/components/AnimatorComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/PointLightComponent.hpp"
#include "spark/ecs/components/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/SpotLightComponent.hpp"
#include "spark/ecs/components/SceneSpatialPolicyComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/SpriteLighting2DComponent.hpp"
#include "spark/ecs/components/SkyComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/scene/SceneDrawableFrustumSink.hpp"
#include "spark/scene/ScenePartitionKind.hpp"
#include "spark/scene/DrawableSortResolver.hpp"
#include "spark/scene/SceneSpriteTileCull.hpp"
#include "spark/scene/SceneTilemapSubmit.hpp"
#include "spark/render/SceneLightingProfile.hpp"
#include "spark/scene/Texture2D.hpp"

#include "spark/math/Constants.hpp"

#include <functional>

namespace Spark {

namespace {

struct RigidDrawableSubmitSink final : DrawableFrustumSink {
    Array<SceneDrawItem>& drawList;
    SceneRenderParams& params;
    const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& findOrAddTexture;
    std::int32_t defaultShadowFlags = 0;

    RigidDrawableSubmitSink(
            Array<SceneDrawItem>& inDrawList,
            SceneRenderParams& inParams,
            const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& inFindTex,
            const std::int32_t inDefaultShadowFlags) noexcept
        : drawList(inDrawList),
          params(inParams),
          findOrAddTexture(inFindTex),
          defaultShadowFlags(inDefaultShadowFlags) {}

    void OnDrawable(
            GameObject* o,
            const MeshComponent& mc,
            const MaterialComponent* mat,
            const Matrix4& worldM) override {
        if (o == nullptr || !mc.GetMesh()) {
            return;
        }
        const SkyComponent* sky = o->GetComponent<SkyComponent>();
        if (sky != nullptr && sky->IsSkyEnabled()) {
            SceneDrawItem item{};
            item.mesh = SceneMeshSlot::Custom;
            item.skyMode = sky->GetSkyMode();
            item.model = worldM;
            item.customMesh = mc.GetMesh();
            item.albedo = sky->GetTint();
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 1.0F;
            if (mat != nullptr && mat->GetBaseColorTexture()) {
                const Vector3& t = mat->GetTint();
                item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
            }
            item.shadowFlags = 0;
            drawList.PushBack(item);
            return;
        }
        SceneDrawItem item{};
        item.model = worldM;
        item.mesh = mc.GetSlot();
        if (mc.GetSlot() == SceneMeshSlot::Custom) {
            item.customMesh = mc.GetMesh();
        }
        Vector3 alb = mc.GetAlbedo();
        item.textureLayer = -1;
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
            if (mat->GetBaseColorTexture()) {
                const Vector3& t = mat->GetTint();
                alb = {alb.x * t.x, alb.y * t.y, alb.z * t.z};
                item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
            }
        }
        item.albedo = alb;
        if (mc.GetSlot() == SceneMeshSlot::GroundPlane) {
            item.doubleSided = true;
        }
        item.shadowFlags = defaultShadowFlags;
        drawList.PushBack(item);
    }
};

}  // namespace

void FillStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        float lightIntensity,
        const Vector3& ambientColor,
        bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        const float sceneTimeSeconds,
        SceneRenderParams& params,
        const SceneSpriteSortMode spriteSortMode,
        const Scene* sceneForCulling) {
    params.sceneTimeSeconds = sceneTimeSeconds;
    params.spriteSortMode = spriteSortMode;
    params.viewProjection = viewProjection;
    params.cameraPositionWorld = cameraPositionWorld;
    params.lightDirectionWorld = lightDirectionWorld.Normalized();
    params.lightColor = lightColor;
    params.lightIntensity = lightIntensity;
    params.ambientColor = ambientColor;
    SceneSubmitDetail::ApplyEcsDirectionalLight(world, params);

    params.draws.Clear();
    params.transparentDraws.Clear();
    params.sceneTextures.Clear();
    params.pointLights.Clear();
    params.spotLights.Clear();
    params.screenRects.Clear();
    params.screenTexts.Clear();
    params.screenOverlayRects.Clear();
    params.screenOverlayTexts.Clear();
    params.screenLateRects.Clear();
    params.screenLateTexts.Clear();
    params.uiPaintOrderNext = 0U;
    params.sprites.Clear();
    params.tilemaps.Clear();
    params.tilemapTiles.Clear();
    params.uiFont = world.GetUiFont();
    params.uiBoldFont = world.GetUiBoldFont();
    params.draws.Reserve(32);

    const ResolvedSceneLighting resolvedLighting = ResolveSceneLightingFromParams(
            params.lightingProfile,
            params.exposure,
            params.shadowCascadeNear,
            params.shadowCascadeFar,
            params.shadowDistanceMax,
            params.shadowFadeStartRatio,
            params.ambientScale,
            params.directionalShadowsEnabled,
            params.shadowsCastByDefault,
            params.shadowsReceiveByDefault,
            params.useTimeOfDay,
            params.timeOfDay);
    const std::int32_t defaultShadowFlags = DefaultShadowFlagsFor(resolvedLighting);

    world.ForEachGameObject([&params](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const PointLightComponent* pl = o->GetComponent<PointLightComponent>();
        if (pl == nullptr || !pl->IsEnabled()) {
            return;
        }
        if (params.pointLights.GetSize() >= SceneRenderParams::MaxPointLights) {
            return;
        }
        const Matrix4 worldMat = o->GetWorldMatrix();
        ScenePointLight gpu{};
        gpu.positionWorld = worldMat.TranslationVector();
        gpu.range = pl->GetRange();
        gpu.color = pl->GetColor();
        gpu.intensity = pl->GetIntensity();
        gpu.castsShadow = pl->CastsShadow();
        params.pointLights.PushBack(gpu);
    });

    world.ForEachGameObject([&params](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const SpotLightComponent* sl = o->GetComponent<SpotLightComponent>();
        if (sl == nullptr || !sl->IsEnabled()) {
            return;
        }
        if (params.spotLights.GetSize() >= SceneRenderParams::MaxSpotLights) {
            return;
        }
        const Matrix4 worldM = o->GetWorldMatrix();
        Vector3 axis = worldM.TransformVector(Vector3{0.0F, 0.0F, -1.0F});
        if (axis.LengthSquared() < 1e-10F) {
            axis = {0.0F, -1.0F, 0.0F};
        } else {
            axis = axis.Normalized();
        }
        SceneSpotLight gpu{};
        gpu.positionWorld = worldM.TranslationVector();
        gpu.range = sl->GetRange();
        gpu.directionWorld = axis;
        const float innerRad = DegreesToRadians(sl->GetInnerConeDegrees());
        float outerRad = DegreesToRadians(sl->GetOuterConeDegrees());
        if (outerRad < innerRad) {
            outerRad = innerRad;
        }
        gpu.innerConeRadians = innerRad;
        gpu.outerConeRadians = outerRad;
        gpu.color = sl->GetColor();
        gpu.intensity = sl->GetIntensity();
        gpu.castsShadow = sl->CastsShadow();
        params.spotLights.PushBack(gpu);
    });

    auto findOrAddTexture = [&params](const SharedPtr<Texture2D>& tex) -> std::int32_t {
        return SceneSubmitDetail::FindOrAddSceneTexture(params, tex);
    };

    Array<SceneDrawItem> drawList;
    drawList.Reserve(48);

    RigidDrawableSubmitSink rigidSink{drawList, params, findOrAddTexture, defaultShadowFlags};
    if (sceneForCulling != nullptr) {
        DispatchDrawableFrustumCull(
                *sceneForCulling, viewProjection, sceneForCulling->GetSpatialPartitionKind(), rigidSink);
    } else {
        world.ForEachGameObject([&](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const MeshComponent* mc = o->GetComponent<MeshComponent>();
            if (mc == nullptr || !mc->GetMesh()) {
                return;
            }
            rigidSink.OnDrawable(o, *mc, o->GetComponent<MaterialComponent>(), o->GetWorldMatrix());
        });
    }

    ScenePartitionKind skinnedPartition = ScenePartitionKind::None;
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr || skinnedPartition != ScenePartitionKind::None) {
            return;
        }
        if (const SceneSpatialPolicyComponent* policy = o->GetComponent<SceneSpatialPolicyComponent>()) {
            skinnedPartition = policy->GetPartitionKind();
        }
    });

    struct SkinnedSubmitSink final : SkinnedDrawableFrustumSink {
        Array<SceneDrawItem>& draws;
        SceneRenderParams& params;
        const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& findTex;
        std::int32_t defaultShadowFlags = 0;

        SkinnedSubmitSink(
                Array<SceneDrawItem>& inDraws,
                SceneRenderParams& inParams,
                const std::function<std::int32_t(const SharedPtr<Texture2D>&)>& inFindTex,
                const std::int32_t inDefaultShadowFlags) noexcept
            : draws(inDraws),
              params(inParams),
              findTex(inFindTex),
              defaultShadowFlags(inDefaultShadowFlags) {}

        void OnSkinnedDrawable(GameObject* /*object*/,
                const SkinnedMeshComponent& smc,
                const MaterialComponent* mat,
                const AnimatorComponent* anim,
                const Matrix4& world) override {
            if (!smc.GetMesh() || anim == nullptr || !anim->GetSkeleton()) {
                return;
            }
            const std::uint32_t jc = anim->GetSkeleton()->GetJointCount();
            if (jc == 0) {
                return;
            }
            SceneDrawItem item{};
            item.model = world;
            item.mesh = SceneMeshSlot::Custom;
            item.skinnedMesh = smc.GetMesh();
            item.albedo = {0.9F, 0.88F, 0.82F};
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 0.5F;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Vector3& t = mat->GetTint();
                    item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                    item.textureLayer = findTex(mat->GetBaseColorTexture());
                }
            }
            item.jointPalette.Resize(jc);
            anim->ComputeJointPalette(item.jointPalette.GetData(), Skeleton::MaxJoints);
            item.shadowFlags = defaultShadowFlags;
            draws.PushBack(item);
        }
    };

    SkinnedSubmitSink skinnedSink{drawList, params, findOrAddTexture, defaultShadowFlags};
    DispatchSkinnedDrawableFrustumCull(world, viewProjection, skinnedPartition, skinnedSink);

    SceneSubmitDetail::StableSortDrawItems(drawList);
    PartitionSortedDrawItemsIntoSceneParams(drawList, params, cameraPositionWorld);

    const SceneSpriteTileCull spriteTileCull(viewProjection);
    const SceneTilemapSubmitter tilemapSubmitter{};

    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const SpriteComponent* sc = o->GetComponent<SpriteComponent>();
        if (sc == nullptr) {
            return;
        }
        if (params.sprites.GetSize() >= SceneRenderParams::MaxSprites) {
            return;
        }
        const Matrix4 spriteModel = o->GetWorldMatrix();
        if (!spriteTileCull.IsSpriteVisible(spriteModel)) {
            return;
        }
        SceneSpriteDraw sd{};
        sd.model = spriteModel;
        sd.tint = sc->GetTint();
        sd.uvRect = sc->GetUvRect();
        const ResolvedDrawableSort resolved = DrawableSortResolver::Resolve(*o, sc->GetSortOrder());
        sd.sortOrder = resolved.key.sortingOrder;
        sd.sortingLayerOrder = resolved.key.sortingLayerOrder;
        sd.sortWorldY = resolved.worldYAnchor;
        if (sc->GetTexture()) {
            sd.textureLayer = findOrAddTexture(sc->GetTexture());
        } else {
            sd.textureLayer = -1;
        }
        if (const SpriteLighting2DComponent* lit = o->GetComponent<SpriteLighting2DComponent>()) {
            sd.lightingMode = lit->GetMode();
            sd.lightingParam0 = lit->GetParam0();
            sd.lightingParam1 = lit->GetParam1();
        }
        sd.blendMode = SceneSubmitDetail::ResolveSpriteBlendMode(*o);
        params.sprites.PushBack(sd);
    });

    tilemapSubmitter.Submit(world, params, spriteTileCull, findOrAddTexture, SceneSubmitDetail::ResolveSpriteBlendMode);

    SceneSubmitDetail::StableSortSprites(params.sprites, params.spriteSortMode);

    params.particles.Clear();
    if (enableParticles) {
        params.particleCameraRight = particleCameraRight.Normalized();
        params.particleCameraUp = particleCameraUp.Normalized();
        world.ForEachGameObject([&params](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            if (params.particles.GetSize() >= SceneRenderParams::MaxParticles) {
                return;
            }
            const ParticleEmitterComponent* pe = o->GetComponent<ParticleEmitterComponent>();
            if (pe == nullptr || !pe->IsEmitterEnabled()) {
                return;
            }
            Array<SceneParticleInstance> chunk;
            pe->CollectInstances(chunk);
            for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
                if (params.particles.GetSize() >= SceneRenderParams::MaxParticles) {
                    return;
                }
                params.particles.PushBack(chunk[ci]);
            }
        });
    }

    world.ForEachGameObject([&params](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const TextOverlayComponent* tc = o->GetComponent<TextOverlayComponent>();
        if (tc == nullptr || !tc->IsVisible()) {
            return;
        }
        ScreenTextDraw d{};
        d.text = tc->GetText();
        d.x = tc->GetScreenX();
        d.y = tc->GetScreenY();
        d.sizePixels = tc->GetFontSizePixels();
        d.color = tc->GetColor();
        d.alpha = tc->GetAlpha();
        d.paintOrder = params.NextUiPaintOrder();
        params.screenTexts.PushBack(MoveTemp(d));
    });

    SceneSubmitDetail::ResolveIblEnvironmentLayer(params);

    if (params.draws.IsEmpty() && (!params.sprites.IsEmpty() || !params.tilemaps.IsEmpty())) {
        params.directionalShadowsEnabled = false;
        params.punctualShadowsEnabled = false;
        params.ssaoEnabled = false;
    }
}

void SubmitStandardLitSceneFromWorld(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp,
        const float sceneTimeSeconds,
        const SceneSpriteSortMode spriteSortMode) {
    SceneRenderParams params{};
    FillStandardLitSceneFromWorld(world,
            context,
            viewProjection,
            cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            enableParticles,
            particleCameraRight,
            particleCameraUp,
            sceneTimeSeconds,
            params,
            spriteSortMode);
    context.SetSceneRenderParams(params);
}

bool TryFillSceneCameraFromWorld(
        const GameWorld& world,
        const float framebufferWidth,
        const float framebufferHeight,
        SceneRenderParams& outParams) noexcept {
    SceneCameraMatrices cam{};
    if (!TryBuildSceneCameraMatrices(world, framebufferWidth, framebufferHeight, cam)) {
        return false;
    }
    outParams.viewProjection = cam.viewProjection;
    outParams.cameraPositionWorld = cam.positionWorld;
    outParams.particleCameraRight = cam.rightWorld;
    outParams.particleCameraUp = cam.upWorld;
    return true;
}

bool SubmitStandardLitSceneFromWorldWithCamera(
        GameWorld& world,
        IEngineContext& context,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const bool enableParticles,
        const float sceneTimeSeconds,
        const SceneSpriteSortMode spriteSortMode) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    SceneRenderParams params{};
    if (!TryFillSceneCameraFromWorld(
                world,
                static_cast<float>(fbW),
                static_cast<float>(fbH),
                params)) {
        return false;
    }
    FillStandardLitSceneFromWorld(
            world,
            context,
            params.viewProjection,
            params.cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            enableParticles,
            params.particleCameraRight,
            params.particleCameraUp,
            sceneTimeSeconds,
            params,
            spriteSortMode);
    context.SetSceneRenderParams(params);
    return true;
}

}  // namespace Spark
