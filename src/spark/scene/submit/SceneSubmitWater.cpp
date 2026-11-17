#include "spark/scene/submit/SceneWaterSubmit.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"
#include "spark/scene/submit/SceneRigidMeshDrawBuilder.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/water/WaterRenderingProfile.hpp"

namespace Spark {

namespace {

[[nodiscard]] float SquaredDistanceFromCamera(const Matrix4& world, const Vector3& cameraPositionWorld) noexcept {
    const float dx = world.m[12] - cameraPositionWorld.x;
    const float dy = world.m[13] - cameraPositionWorld.y;
    const float dz = world.m[14] - cameraPositionWorld.z;
    return dx * dx + dy * dy + dz * dz;
}

}  // namespace

void SceneWaterSubmit::PushMeshDraws(
        Array<SceneWaterDraw>& outDraws,
        SceneDrawItem baseItem,
        const Mesh& mesh,
        const MaterialComponent* mat,
        const MultiMaterialComponent* multiMat,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
        const float sortDepth,
        const float waveTimeSeconds,
        const WaterWaveSettings& waveSettings,
        const float waterLevelY,
        const Vector3& deepColor,
        const float absorption,
        const float foamStrength,
        const float detailNormalStrength,
        const float shorelineFoamStrength,
        const float shorelineFoamMaxDepth,
        const Vector2 waterTileAnchorXZ,
        const WaterScreenSpaceReflectionSettings& ssrSettings) const {
    Array<SceneDrawItem> meshItems;
    SceneRigidMeshDrawBuilder builder(findOrAddTexture, params);
    builder.AppendRigidMeshDraws(meshItems, baseItem, mesh, mat, multiMat);
    for (std::size_t i = 0; i < meshItems.GetSize(); ++i) {
        SceneWaterDraw draw{};
        draw.sortDepth = sortDepth;
        draw.waveTimeSeconds = waveTimeSeconds;
        draw.waveSettings = waveSettings;
        draw.waterLevelY = waterLevelY;
        draw.deepColor = deepColor;
        draw.absorption = absorption;
        draw.foamStrength = foamStrength;
        draw.detailNormalStrength = detailNormalStrength;
        draw.shorelineFoamStrength = shorelineFoamStrength;
        draw.shorelineFoamMaxDepth = shorelineFoamMaxDepth;
        draw.waterTileAnchorXZ = waterTileAnchorXZ;
        draw.ssrSettings = ssrSettings;
        draw.item = meshItems[i];
        if (mat != nullptr) {
            draw.item.albedo = baseItem.albedo;
        }
        outDraws.PushBack(draw);
    }
}

void SceneWaterSubmit::AppendBodyDraw(
        GameObject& owner,
        const WaterBodyComponent& water,
        const MeshComponent& mesh,
        const MaterialComponent* mat,
        const Matrix4& worldM,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
        const Vector3& cameraPositionWorld) const {
    if (params.waterDraws.GetSize() >= SceneRenderParams::MaxWaterDraws) {
        return;
    }

    const float sortDepth = SquaredDistanceFromCamera(worldM, cameraPositionWorld);
    const float waterLevelY = water.GetWaterLevelY();
    const Vector3 deepColor = water.GetResolvedDeepColor(mat);
    const float absorption = water.GetAbsorption();
    const float foamStrength = water.GetFoamStrength();
    const float detailNormalStrength = water.GetDetailNormalStrength();
    const float shorelineFoamStrength = water.GetShorelineFoamStrength();
    const float shorelineFoamMaxDepth = water.GetShorelineFoamMaxDepth();
    const Vector2 waterTileAnchorXZ = water.GetSurfaceTileAnchorXZ();
    const WaterScreenSpaceReflectionSettings& ssrSettings = water.GetSsrSettings();
    SceneDrawItem baseItem{};
    baseItem.model = worldM;
    baseItem.mesh = mesh.GetSlot();
    baseItem.albedo = water.GetResolvedSurfaceAlbedo(mat);
    baseItem.textureLayer = -1;
    baseItem.doubleSided = false;
    baseItem.shadowFlags = SceneSubmitDetail::ResolveWaterShadowFlags(mat, params);
    if (mesh.GetSlot() == SceneMeshSlot::Custom) {
        baseItem.customMesh = mesh.GetMesh();
    }

    const MultiMaterialComponent* multiMat = owner.GetComponent<MultiMaterialComponent>();
    if (mesh.GetSlot() == SceneMeshSlot::Custom && mesh.GetMesh() && multiMat != nullptr &&
        !mesh.GetMesh()->GetSubmeshes().IsEmpty()) {
        PushMeshDraws(
                params.waterDraws,
                baseItem,
                *mesh.GetMesh(),
                mat,
                multiMat,
                params,
                findOrAddTexture,
                sortDepth,
                water.GetWaveTimeSeconds(),
                water.GetResolvedWaveSettings(),
                waterLevelY,
                deepColor,
                absorption,
                foamStrength,
                detailNormalStrength,
                shorelineFoamStrength,
                shorelineFoamMaxDepth,
                waterTileAnchorXZ,
                ssrSettings);
        return;
    }

    SceneWaterDraw draw{};
    draw.sortDepth = sortDepth;
    draw.item = baseItem;
    draw.waveTimeSeconds = water.GetWaveTimeSeconds();
    draw.waveSettings = water.GetResolvedWaveSettings();
    draw.waterLevelY = waterLevelY;
    draw.deepColor = deepColor;
    draw.absorption = absorption;
    draw.foamStrength = foamStrength;
    draw.detailNormalStrength = detailNormalStrength;
    draw.shorelineFoamStrength = shorelineFoamStrength;
    draw.shorelineFoamMaxDepth = shorelineFoamMaxDepth;
    draw.waterTileAnchorXZ = waterTileAnchorXZ;
    draw.ssrSettings = ssrSettings;
    if (mat != nullptr) {
        ApplyMaterialComponentToSceneDrawItem(draw.item, mat, &params);
        SceneSubmitDetail::ApplyAlbedoTexture(draw.item, mat->GetBaseColorTexture(), Vector3::One, findOrAddTexture);
        draw.item.albedo = baseItem.albedo;
    }
    params.waterDraws.PushBack(draw);
}

void SceneWaterSubmit::SubmitFromWorld(
        GameWorld& world,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
        const Scene* sceneForCulling) {
    params.waterDraws.Clear();
    params.waterDraws.Reserve(4);

    auto submitOne = [&](GameObject& owner,
                             const WaterBodyComponent& water,
                             const MeshComponent& mesh,
                             const MaterialComponent* mat,
                             const Matrix4& worldM) {
        AppendBodyDraw(owner, water, mesh, mat, worldM, params, findOrAddTexture, cameraPositionWorld);
    };

    if (sceneForCulling != nullptr) {
        sceneForCulling->ForEachWaterBodyInViewFrustum(viewProjection, submitOne);
    } else {
        world.Entities().ForEachActive([&](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const WaterBodyComponent* water = o->GetComponent<WaterBodyComponent>();
            if (water == nullptr) {
                return;
            }
            const MeshComponent* mesh = o->GetComponent<MeshComponent>();
            if (mesh == nullptr || !mesh->GetMesh()) {
                return;
            }
            submitOne(*o, *water, *mesh, o->GetComponent<MaterialComponent>(), o->GetWorldMatrix());
        });
    }

    SceneDrawListSorter::StableSortWaterBackToFront(
            params.waterDraws, cameraPositionWorld, params.waterSortMode);

    for (std::size_t di = 0; di < params.waterDraws.GetSize(); ++di) {
        SceneWaterDraw& draw = params.waterDraws[di];
        draw.resolvedSsr = ResolveWaterScreenSpaceReflection(draw.ssrSettings, params.waterRenderingProfile);
    }
}

}  // namespace Spark
