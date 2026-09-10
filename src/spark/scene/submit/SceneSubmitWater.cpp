#include "spark/scene/submit/SceneWaterSubmit.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

float SceneWaterSubmit::SquaredDistanceFromCamera(
        const Matrix4& world,
        const Vector3& cameraPositionWorld) noexcept {
    const float dx = world.m[12] - cameraPositionWorld.x;
    const float dy = world.m[13] - cameraPositionWorld.y;
    const float dz = world.m[14] - cameraPositionWorld.z;
    return dx * dx + dy * dy + dz * dz;
}

void SceneWaterSubmit::SortDrawsBackToFront(
        Array<SceneWaterDraw>& items,
        const Vector3& cameraPositionWorld) const {
    const auto fartherFirst = [&cameraPositionWorld](const SceneWaterDraw& a, const SceneWaterDraw& b) noexcept -> bool {
        if (a.sortDepth != b.sortDepth) {
            return a.sortDepth > b.sortDepth;
        }
        return SquaredDistanceFromCamera(a.item.model, cameraPositionWorld) >
               SquaredDistanceFromCamera(b.item.model, cameraPositionWorld);
    };
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneWaterDraw key = items[i];
        std::size_t j = i;
        while (j > 0 && fartherFirst(items[j - 1], key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

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
        const WaterWaveSettings& waveSettings) const {
    const Array<MeshSubmesh>& submeshes = mesh.GetSubmeshes();
    if (submeshes.IsEmpty() || multiMat == nullptr) {
        SceneWaterDraw draw{};
        draw.sortDepth = sortDepth;
        draw.waveTimeSeconds = waveTimeSeconds;
        draw.waveSettings = waveSettings;
        draw.item = baseItem;
        draw.item.submeshIndex = kSceneDrawFullSubmesh;
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(draw.item, mat, &params);
            SceneSubmitDetail::ApplyAlbedoTexture(
                    draw.item, mat->GetBaseColorTexture(), Vector3::One, findOrAddTexture);
            draw.item.albedo = baseItem.albedo;
        } else if (multiMat != nullptr && multiMat->GetSlotCount() > 0U) {
            const MultiMaterialComponent::Slot& slot = multiMat->GetSlot(0U);
            ApplyMultiMaterialSlotToSceneDrawItem(draw.item, slot, &params);
            SceneSubmitDetail::ApplyAlbedoTexture(draw.item, slot.baseColor, slot.tint, findOrAddTexture);
        }
        outDraws.PushBack(draw);
        return;
    }

    for (std::size_t si = 0; si < submeshes.GetSize(); ++si) {
        SceneWaterDraw draw{};
        draw.sortDepth = sortDepth;
        draw.waveTimeSeconds = waveTimeSeconds;
        draw.waveSettings = waveSettings;
        draw.item = baseItem;
        draw.item.submeshIndex = static_cast<std::uint32_t>(si);
        const MeshSubmesh& sm = submeshes[si];
        const std::uint32_t materialIndex = sm.ResolveMaterialIndex(multiMat->GetActiveVariantIndex());
        if (materialIndex < multiMat->GetSlotCount()) {
            const MultiMaterialComponent::Slot& slot = multiMat->GetSlot(materialIndex);
            ApplyMultiMaterialSlotToSceneDrawItem(draw.item, slot, &params);
            SceneSubmitDetail::ApplyAlbedoTexture(draw.item, slot.baseColor, slot.tint, findOrAddTexture);
        } else if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(draw.item, mat, &params);
            SceneSubmitDetail::ApplyAlbedoTexture(
                    draw.item, mat->GetBaseColorTexture(), Vector3::One, findOrAddTexture);
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
    SceneDrawItem baseItem{};
    baseItem.model = worldM;
    baseItem.mesh = mesh.GetSlot();
    baseItem.albedo = water.GetResolvedSurfaceAlbedo(mat);
    baseItem.textureLayer = -1;
    baseItem.doubleSided = false;
    baseItem.shadowFlags = 0;
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
                water.GetResolvedWaveSettings());
        return;
    }

    SceneWaterDraw draw{};
    draw.sortDepth = sortDepth;
    draw.item = baseItem;
    draw.waveTimeSeconds = water.GetWaveTimeSeconds();
    draw.waveSettings = water.GetResolvedWaveSettings();
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
        world.ForEachActiveGameObject([&](GameObject* o) {
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

    SortDrawsBackToFront(params.waterDraws, cameraPositionWorld);
}

namespace SceneSubmitDetail {

void SubmitWaterBodiesFromWorld(
        GameWorld& world,
        const Matrix4& viewProjection,
        const Vector3& cameraPositionWorld,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
        const Scene* sceneForCulling) {
    SceneWaterSubmit submitter{};
    submitter.SubmitFromWorld(world, viewProjection, cameraPositionWorld, params, findOrAddTexture, sceneForCulling);
}

}  // namespace SceneSubmitDetail

}  // namespace Spark
