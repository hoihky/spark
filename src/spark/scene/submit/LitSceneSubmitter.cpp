#include "spark/scene/submit/LitSceneSubmitter.hpp"

#include "spark/scene/submit/Scene2DSubmitCaps.hpp"
#include "spark/scene/submit/detail/SceneSubmitLighting.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/animation/SkinnedAnimationBudgetComponent.hpp"
#include "spark/ecs/components/lighting/PointLight2DComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/rendering/DecalProjectorComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Constants.hpp"
#include "spark/render/lighting/SceneLightingResolver.hpp"
#include "spark/render/sprites2d/Scene2DComposite.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/core/SceneDrawableFrustumSink.hpp"
#include "spark/scene/core/ScenePartitionKind.hpp"
#include "spark/scene/foliage/FoliageSceneCollector.hpp"
#include "spark/scene/foliage/WindSubsystem.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"
#include "spark/scene/submit/SceneMeshSubmitCollectors.hpp"
#include "spark/scene/submit/SceneSpriteTileCull.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/SceneTilemapSubmit.hpp"
#include "spark/scene/submit/SceneWaterSubmit.hpp"
#include "spark/scene/submit/SkinnedAnimationService.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/volume/RenderVolumes.hpp"
#if SPARK_HAS_GAMEPLAY
#include "spark/ecs/components/gameplay/FogOfWar2DComponent.hpp"
#endif

namespace Spark {

void LitSceneSubmitter::ResetSceneParams(
        GameWorld& world,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const Vector3& lightDirectionWorld,
        const Vector3& lightColor,
        const float lightIntensity,
        const Vector3& ambientColor,
        const float sceneTimeSeconds,
        const SceneSpriteSortMode spriteSortMode,
        SceneRenderParams& params) {
    params.sceneTimeSeconds = sceneTimeSeconds;
    params.spriteSortMode = spriteSortMode;
    params.viewProjection = viewProjection;
    params.cameraPositionWorld = cameraPositionWorld;
    params.lightDirectionWorld = lightDirectionWorld.Normalized();
    params.lightColor = lightColor;
    params.lightIntensity = lightIntensity;
    params.ambientColor = ambientColor;

    params.draws.Clear();
    params.transparentDraws.Clear();
    params.waterDraws.Clear();
    params.sceneTextures.Clear();
    params.sceneHdrTextures.Clear();
    params.pointLights.Clear();
    params.spotLights.Clear();
    params.screenRects.Clear();
    params.screenSprites.Clear();
    params.uiTextures.Clear();
    params.screenTexts.Clear();
    params.screenOverlayRects.Clear();
    params.screenOverlaySprites.Clear();
    params.screenOverlayTexts.Clear();
    params.screenLateRects.Clear();
    params.screenLateSprites.Clear();
    params.screenLateTexts.Clear();
    params.uiPaintOrderNext = 0U;
    params.sprites.Clear();
    params.tilemaps.Clear();
    params.tilemapTiles.Clear();
    params.foliageBatches.Clear();
    params.foliageInstances.Clear();
    params.uiFont = world.GetUiFont();
    params.uiBoldFont = world.GetUiBoldFont();
    params.draws.Reserve(32);
}

void LitSceneSubmitter::AdvanceTimeOfDay(GameWorld& world, const float sceneTimeSeconds) {
    static float sLastSceneTime = 0.0F;
    float dt = sceneTimeSeconds - sLastSceneTime;
    if (dt < 0.0F || dt > 1.0F) {
        dt = 0.0F;
    }
    sLastSceneTime = sceneTimeSeconds;
    ProcessTimeOfDayDrivers(world, dt);
}

void LitSceneSubmitter::CollectPunctualLights(GameWorld& world, SceneRenderParams& params) const {
    world.Entities().ForEachActive([&params](GameObject* o) {
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

    world.Entities().ForEachActive([&params](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const PointLight2DComponent* light2d = o->GetComponent<PointLight2DComponent>();
        if (light2d == nullptr || !light2d->IsEnabled()) {
            return;
        }
        if (params.pointLights.GetSize() >= SceneRenderParams::MaxPointLights) {
            return;
        }
        const Matrix4 worldMat = o->GetWorldMatrix();
        const Vector3 translation = worldMat.TranslationVector();
        const Vector2 offset = light2d->GetLocalOffset();
        ScenePointLight gpu{};
        gpu.positionWorld = {translation.x + offset.x, translation.y + offset.y, light2d->GetWorldZ()};
        gpu.range = light2d->GetRange();
        gpu.color = light2d->GetColor();
        gpu.intensity = light2d->GetEffectiveIntensity();
        gpu.castsShadow = false;
        params.pointLights.PushBack(gpu);
    });

    world.Entities().ForEachActive([&params](GameObject* o) {
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
}

SceneSubmitDetail::FindSceneTextureFn LitSceneSubmitter::MakeTextureResolver(
        GameWorld& world,
        SceneRenderParams& params) const {
    return [&params, &world](const SharedPtr<Texture2D>& tex, Vector2* uvScale, Vector2* uvOffset) -> std::int32_t {
        if (tex && !tex->HasAtlasBinding() &&
            params.sceneTextures.GetSize() + 4U >= SceneRenderParams::MaxSceneTextures) {
            (void)world.Assets().TryAutoPackTextureIntoAtlas("scene_auto", tex);
        }
        return SceneSubmitDetail::FindOrAddSceneTexture(params, tex, uvScale, uvOffset);
    };
}

void LitSceneSubmitter::CollectMeshDraws(
        GameWorld& world,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture,
        const std::int32_t defaultShadowFlags,
        const Scene* sceneForCulling) const {
    Array<SceneDrawItem> drawList;
    drawList.Reserve(48);

    const bool skiesSubmittedOutsideCull = (sceneForCulling != nullptr);
    if (sceneForCulling != nullptr) {
        sceneForCulling->ForEachSky([&](GameObject&, const SkyComponent& sk, const MeshComponent& mc,
                                             const MaterialComponent* mat, const Matrix4& world) {
            SceneDrawItem item{};
            SceneSubmitDetail::PopulateSkyDrawItem(item, sk, mc, mat, world, params);
            drawList.PushBack(item);
        });
    }

    RigidDrawableSubmitCollector rigidCollector{
            drawList, params, findTexture, defaultShadowFlags, skiesSubmittedOutsideCull};
    if (sceneForCulling != nullptr) {
        DispatchDrawableFrustumCull(
                *sceneForCulling, viewProjection, sceneForCulling->GetSpatialPartitionKind(), rigidCollector);
    } else {
        world.Entities().ForEachActive([&](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const MeshComponent* mc = o->GetComponent<MeshComponent>();
            if (mc == nullptr) {
                return;
            }
            if (!mc->GetMesh() && mc->GetSlot() == SceneMeshSlot::Custom) {
                return;
            }
            rigidCollector.OnDrawable(o, *mc, o->GetComponent<MaterialComponent>(), o->GetWorldMatrix());
        });
    }

    ScenePartitionKind skinnedPartition = ScenePartitionKind::None;
    world.Entities().ForEachActive([&](GameObject* o) {
        if (o == nullptr || skinnedPartition != ScenePartitionKind::None) {
            return;
        }
        if (const SceneSpatialPolicyComponent* policy = o->GetComponent<SceneSpatialPolicyComponent>()) {
            skinnedPartition = policy->GetPartitionKind();
        }
    });

    SkinnedAnimationService& skinnedService = world.GetSkinnedAnimationService();
    skinnedService.ResetPolicyToDefaults();
    bool skinnedBudgetApplied = false;
    world.Entities().ForEachActive([&](GameObject* o) {
        if (skinnedBudgetApplied || o == nullptr) {
            return;
        }
        if (const SkinnedAnimationBudgetComponent* policy = o->GetComponent<SkinnedAnimationBudgetComponent>()) {
            skinnedService.ApplyBudgetPolicy(*policy);
            skinnedBudgetApplied = true;
        }
    });
    static std::uint32_t skinnedSubmitFrameSerial = 0;
    skinnedService.BeginSubmitFrame(++skinnedSubmitFrameSerial);

    SkinnedDrawableSubmitCollector skinnedCollector{world, drawList, params, findTexture, defaultShadowFlags};
    DispatchSkinnedDrawableFrustumCull(world, viewProjection, skinnedPartition, skinnedCollector);

    SceneDrawListSorter::StableSortOpaqueDraws(drawList);
    SceneDrawListSorter::PartitionIntoSceneParams(drawList, params, cameraPositionWorld);

    SceneWaterSubmit waterSubmit{};
    waterSubmit.SubmitFromWorld(
            world, viewProjection, cameraPositionWorld, params, findTexture, sceneForCulling);
}

void LitSceneSubmitter::CollectSpritesTilemapsAndParticles(
        GameWorld& world,
        const Matrix4& viewProjection,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture,
        const bool enableParticles,
        const Vector3& particleCameraRight,
        const Vector3& particleCameraUp) const {
    const SceneSpriteTileCull spriteTileCull(viewProjection);
    const SceneTilemapSubmit tilemapSubmit{};

    world.Entities().ForEachActive([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const SpriteComponent* sc = o->GetComponent<SpriteComponent>();
        if (sc == nullptr) {
            return;
        }
        if (!Scene2DSubmitCaps::HasSpriteCapacity(params)) {
            Scene2DSubmitCaps::NotifySpriteCapacityReached();
            return;
        }
        const Matrix4 spriteModel = o->GetWorldMatrix();
        if (const SharedPtr<Texture2D>& spriteTex = sc->GetTexture()) {
            findTexture(spriteTex, nullptr, nullptr);
        }
        if (!spriteTileCull.IsSpriteVisible(spriteModel)) {
            return;
        }
        SceneSpriteDraw sd{};
        sd.model = spriteModel;
        sd.tint = sc->GetTint();
        sd.uvRect = sc->GetUvRect();
        if (const SharedPtr<Texture2D>& spriteTex = sc->GetTexture()) {
            sd.uvRect = spriteTex->ScaleUvRectForSceneLayer(sd.uvRect);
        }
        const ResolvedDrawableSort resolved = DrawableSortResolver::Resolve(*o, sc->GetSortOrder());
        DrawableSortResolver::ApplyToSpriteDraw(resolved, sd);
        if (sc->GetTexture()) {
            sd.textureLayer = findTexture(sc->GetTexture(), nullptr, nullptr);
        } else {
            sd.textureLayer = -1;
        }
        sd.normalUvRect = sd.uvRect;
        if (const SpriteLighting2DComponent* lit = o->GetComponent<SpriteLighting2DComponent>()) {
            sd.lightingMode = lit->GetMode();
            sd.lightingParam0 = lit->GetParam0();
            sd.lightingParam1 = lit->GetParam1();
            if (!lit->GetSyncNormalUvWithSprite()) {
                sd.normalUvRect = lit->GetNormalUvRect();
                if (const SharedPtr<Texture2D>& normalTex = lit->GetNormalMap()) {
                    sd.normalUvRect = normalTex->ScaleUvRectForSceneLayer(sd.normalUvRect);
                }
            }
            if (const SharedPtr<Texture2D>& normalTex = lit->GetNormalMap()) {
                sd.normalTextureLayer = findTexture(normalTex, nullptr, nullptr);
            }
            if (const SharedPtr<Texture2D>& rampTex = lit->GetRampMap()) {
                sd.rampTextureLayer = findTexture(rampTex, nullptr, nullptr);
            }
        }
        sd.blendMode = SceneDrawListSorter::ResolveSpriteBlendMode(*o);
        params.sprites.PushBack(sd);
    });

    tilemapSubmit.Submit(
            world,
            params,
            spriteTileCull,
            [&](const SharedPtr<Texture2D>& tex) { return findTexture(tex, nullptr, nullptr); },
            SceneDrawListSorter::ResolveSpriteBlendMode);

    SceneDrawListSorter::StableSortSprites(params.sprites, params.spriteSortMode);

    params.particles.Clear();
    if (!enableParticles) {
        return;
    }
    params.particleCameraRight = particleCameraRight.Normalized();
    params.particleCameraUp = particleCameraUp.Normalized();
    world.Entities().ForEachActive([&params, &findTexture](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const ParticleEmitterComponent* pe = o->GetComponent<ParticleEmitterComponent>();
        if (pe == nullptr || !pe->IsEmitterEnabled()) {
            return;
        }
        Array<SceneParticleInstance> chunk;
        pe->CollectInstances(chunk);
        std::int32_t textureLayer = -1;
        if (pe->GetTexture()) {
            textureLayer = findTexture(pe->GetTexture(), nullptr, nullptr);
        }
        if (pe->GetRenderSpace() == ParticleRenderSpace::SpriteLayer) {
            const ResolvedDrawableSort resolved =
                    DrawableSortResolver::Resolve(*o, pe->GetSpriteLayerSortOrder());
            for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
                if (!Scene2DSubmitCaps::HasSpriteCapacity(params)) {
                    Scene2DSubmitCaps::NotifySpriteCapacityReached();
                    return;
                }
                const SceneParticleInstance& particle = chunk[ci];
                SceneSpriteDraw sd{};
                sd.model = Matrix4::Translation(particle.position) * Matrix4::Scale({particle.size, particle.size, 1.0F});
                sd.tint = particle.color;
                sd.uvRect = particle.uvRect;
                sd.textureLayer = particle.textureLayer >= 0 ? particle.textureLayer : textureLayer;
                DrawableSortResolver::ApplyToSpriteDraw(resolved, sd);
                sd.sortWorldY = particle.position.y;
                sd.blendMode = SceneBlendMode::Additive;
                params.sprites.PushBack(sd);
            }
            return;
        }
        for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
            if (!Scene2DSubmitCaps::HasParticleCapacity(params)) {
                Scene2DSubmitCaps::NotifyParticleCapacityReached();
                return;
            }
            chunk[ci].textureLayer = textureLayer;
            params.particles.PushBack(chunk[ci]);
        }
    });
    SceneDrawListSorter::StableSortSprites(params.sprites, params.spriteSortMode);
}

void LitSceneSubmitter::CollectDecalsTextAndExtras(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture) const {
    params.decals.Clear();
    world.Entities().ForEachActive([&params, &findTexture](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (params.decals.GetSize() >= SceneRenderParams::MaxDecals) {
            return;
        }
        const DecalProjectorComponent* decal = o->GetComponent<DecalProjectorComponent>();
        if (decal == nullptr || !decal->IsEnabled()) {
            return;
        }
        SceneDecalDraw draw{};
        draw.projectorWorld = o->GetWorldMatrix();
        const Vector3 size = decal->GetSize();
        draw.halfExtents = {size.x * 0.5F, size.y * 0.5F, size.z * 0.5F};
        draw.opacity = decal->GetOpacity();
        if (decal->GetTexture()) {
            draw.textureLayer = findTexture(decal->GetTexture(), nullptr, nullptr);
        }
        params.decals.PushBack(draw);
    });

    world.Entities().ForEachActive([&params](GameObject* o) {
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
}

void LitSceneSubmitter::FinalizeScene(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findTexture) const {
    world.Assets().FinalizePendingAutoPackAtlas("scene_auto");

    if (params.draws.IsEmpty() && (!params.sprites.IsEmpty() || !params.tilemaps.IsEmpty())) {
        params.directionalShadowsEnabled = false;
        params.punctualShadowsEnabled = false;
        params.ssaoEnabled = false;
    }

    world.GetWindSubsystem().WriteFrameToSceneRenderParams(params);
    FoliageSceneCollector foliageCollector{};
    foliageCollector.CollectInto(world, params, findTexture);

#if SPARK_HAS_GAMEPLAY
    world.Entities().ForEachActive([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        const FogOfWar2DComponent* fog = object->GetComponent<FogOfWar2DComponent>();
        if (fog == nullptr) {
            return;
        }
        const TilemapGameplayGridComponent* grid = object->GetComponent<TilemapGameplayGridComponent>();
        if (grid == nullptr) {
            return;
        }
        fog->SubmitSceneDraw(params, world, grid->GetGridFrame(), findTexture);
    });
#endif
    SceneDrawListSorter::StableSortSprites(params.sprites, params.spriteSortMode);
    CollectScene2DCompositeViews(world, params);
}

void LitSceneSubmitter::Fill(
        GameWorld& world,
        IEngineContext& /*context*/,
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
        SceneRenderParams& params,
        const SceneSpriteSortMode spriteSortMode,
        const Scene* sceneForCulling) {
    ResetSceneParams(world,
            viewProjection,
            cameraPositionWorld,
            lightDirectionWorld,
            lightColor,
            lightIntensity,
            ambientColor,
            sceneTimeSeconds,
            spriteSortMode,
            params);
    AdvanceTimeOfDay(world, sceneTimeSeconds);
    ApplyRegionalRenderVolumes(world, cameraPositionWorld, params);

    const ResolvedSceneLighting resolvedLighting = SceneLightingResolver::Resolve(params);
    SceneSubmitDetail::ApplyEcsDirectionalLight(world, params);
    const std::int32_t defaultShadowFlags = DefaultShadowFlagsFor(resolvedLighting);

    CollectPunctualLights(world, params);
    const SceneSubmitDetail::FindSceneTextureFn findTexture = MakeTextureResolver(world, params);

    CollectMeshDraws(world, viewProjection, cameraPositionWorld, params, findTexture, defaultShadowFlags, sceneForCulling);
    CollectSpritesTilemapsAndParticles(
            world, viewProjection, params, findTexture, enableParticles, particleCameraRight, particleCameraUp);
    CollectDecalsTextAndExtras(world, params, findTexture);
    FinalizeScene(world, params, findTexture);
}

void LitSceneSubmitter::Fill(
        GameWorld& world,
        IEngineContext& context,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        const LitSceneSubmitOptions& options,
        SceneRenderParams& params) {
    Fill(world,
            context,
            viewProjection,
            cameraPositionWorld,
            options.lightDirectionWorld,
            options.lightColor,
            options.lightIntensity,
            options.ambientColor,
            options.enableParticles,
            options.particleCameraRight,
            options.particleCameraUp,
            options.sceneTimeSeconds,
            params,
            options.spriteSortMode,
            options.sceneForCulling);
}

}  // namespace Spark