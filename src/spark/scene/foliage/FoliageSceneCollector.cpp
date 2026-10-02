#include "spark/scene/foliage/FoliageSceneCollector.hpp"
#include "spark/scene/foliage/GrassBladeMesh.hpp"
#include "spark/scene/foliage/SceneFoliageInstancedBatch.hpp"

#include "spark/ecs/components/foliage/FoliageInstancedMeshComponent.hpp"
#include "spark/ecs/components/foliage/GrassFieldComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void FoliageSceneCollector::CollectInto(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture) const {
    params.foliageBatches.Clear();
    params.foliageInstances.Clear();

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        FoliageInstancedMeshComponent* foliageComponent = object->GetComponent<FoliageInstancedMeshComponent>();
        if (foliageComponent == nullptr || !foliageComponent->IsEnabled()) {
            return;
        }
        FoliageInstancedMeshComponent& foliage = *foliageComponent;
        if (params.foliageInstances.GetSize() >= SceneRenderParams::MaxFoliageInstances) {
            return;
        }
        const SharedPtr<Mesh> mesh = foliage.GetBladeMesh();
        if (!mesh) {
            return;
        }
        const std::uint32_t instanceBegin = static_cast<std::uint32_t>(params.foliageInstances.GetSize());
        foliage.AppendInstanceRecords(params.foliageInstances, object->GetWorldMatrix());
        const std::uint32_t added =
                static_cast<std::uint32_t>(params.foliageInstances.GetSize()) - instanceBegin;
        if (added == 0U) {
            return;
        }
        SceneFoliageInstancedBatch batch{};
        batch.SetSourceMesh(mesh);
        batch.SetAlbedoTint(foliage.GetAlbedoTint());
        batch.SetAlphaCutoff(foliage.GetAlphaCutoff());
        batch.SetBladeHeight(GrassBladeMesh::DefaultBladeHeight());
        batch.SetWindBendScale(foliage.GetWindBendScale());
        batch.SetTextureLayer(findOrAddTexture(foliage.GetAlbedoTexture(), nullptr, nullptr));
        batch.SetInstanceRange(instanceBegin, added);
        params.foliageBatches.PushBack(batch);
    });

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        GrassFieldComponent* grassField = object->GetComponent<GrassFieldComponent>();
        if (grassField == nullptr || !grassField->IsEnabled()) {
            return;
        }
        if (params.foliageInstances.GetSize() >= SceneRenderParams::MaxFoliageInstances) {
            return;
        }
        const SharedPtr<Mesh> mesh = grassField->GetBladeMesh();
        if (!mesh) {
            return;
        }
        const std::uint32_t instanceBegin = static_cast<std::uint32_t>(params.foliageInstances.GetSize());
        grassField->AppendInstanceRecords(params.foliageInstances);
        if (params.maxGrassInstancesPerFrame > 0U &&
            params.foliageInstances.GetSize() - instanceBegin > params.maxGrassInstancesPerFrame) {
            params.foliageInstances.Resize(instanceBegin + params.maxGrassInstancesPerFrame);
        }
        const std::uint32_t added =
                static_cast<std::uint32_t>(params.foliageInstances.GetSize()) - instanceBegin;
        if (added == 0U) {
            return;
        }
        SceneFoliageInstancedBatch batch{};
        batch.SetSourceMesh(mesh);
        batch.SetAlbedoTint(grassField->GetScatterSettings().GetAlbedoTint());
        batch.SetAlphaCutoff(grassField->GetAlphaCutoff());
        batch.SetBladeHeight(GrassBladeMesh::DefaultBladeHeight());
        batch.SetWindBendScale(grassField->GetWindBendScale());
        const GrassChunkScatterSettings& scatter = grassField->GetScatterSettings();
        batch.SetDistanceFadeMeters(scatter.GetDistanceFadeStartMeters(), scatter.GetDistanceFadeEndMeters());
        batch.SetTextureLayer(findOrAddTexture(grassField->GetAlbedoTexture(), nullptr, nullptr));
        batch.SetInstanceRange(instanceBegin, added);
        params.foliageBatches.PushBack(batch);
    });
}


}  // namespace Spark
