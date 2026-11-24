#include "spark/scene/submit/MeshInstancedSceneCollector.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/RigidMeshInstancedBatchComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/mesh/SceneMeshInstancedBatch.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"
#include "spark/scene/submit/SceneRigidMeshDrawBuilder.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/SceneSubmitMaterials.hpp"

namespace Spark {

void MeshInstancedSceneCollector::CollectInto(
        GameWorld& world,
        SceneRenderParams& params,
        const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture) const {
    params.meshInstancedBatches.Clear();
    params.meshInstances.Clear();

    world.Entities().ForEachActive([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        auto* batchComponent = object->GetComponent<RigidMeshInstancedBatchComponent>();
        const MeshComponent* meshComponent = object->GetComponent<MeshComponent>();
        if (batchComponent == nullptr || meshComponent == nullptr || !batchComponent->IsEnabled()) {
            return;
        }
        if (params.meshInstances.GetSize() >= SceneRenderParams::MaxRigidMeshInstances) {
            return;
        }

        const std::uint32_t instanceBegin = static_cast<std::uint32_t>(params.meshInstances.GetSize());
        batchComponent->AppendInstanceRecords(params.meshInstances, object->GetWorldMatrix());
        const std::uint32_t added =
                static_cast<std::uint32_t>(params.meshInstances.GetSize()) - instanceBegin;
        if (added == 0U) {
            return;
        }

        SceneDrawItem templateItem{};
        templateItem.model = Matrix4::Identity;
        templateItem.mesh = meshComponent->GetSlot();
        templateItem.albedo = meshComponent->GetAlbedo();
        if (templateItem.mesh == SceneMeshSlot::Custom) {
            templateItem.customMesh = meshComponent->GetMesh();
        }
        if (templateItem.mesh == SceneMeshSlot::GroundPlane) {
            templateItem.doubleSided = true;
        }
        const MaterialComponent* mat = object->GetComponent<MaterialComponent>();
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(templateItem, mat, &params);
            SceneRigidMeshDrawBuilder::ApplyAlbedoTexture(
                    templateItem, mat->GetBaseColorTexture(), mat->GetTint(), findOrAddTexture);
        }
        templateItem.opacity = 1.0F;
        templateItem.alphaBlend = false;
        DrawableSortResolver::ApplyToSceneDrawItem(*object, templateItem, 0);

        SceneMeshInstancedBatch batch{};
        batch.SetDrawTemplate(templateItem);
        batch.SetInstanceRange(instanceBegin, added);
        params.meshInstancedBatches.PushBack(batch);
    });
}

}  // namespace Spark