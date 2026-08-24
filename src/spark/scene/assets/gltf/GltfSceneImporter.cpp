#include "spark/scene/assets/gltf/GltfSceneImporter.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

namespace {

void ApplyLocalTransform(GameObject& object, const Transform& local) {
    TransformComponent* transform = object.GetComponent<TransformComponent>();
    if (transform == nullptr) {
        transform = object.AddComponent<TransformComponent>();
    }
    transform->SetLocalTransform(local);
}

GltfAsset MakeAssetView(const GltfSceneDocument& document, const GltfSceneNode& node) {
    GltfAsset asset{};
    if (node.HasMesh() && node.meshIndex < document.meshes.GetSize()) {
        asset.mesh = document.meshes[node.meshIndex];
    }
    asset.materials = document.materials;
    if (!asset.materials.IsEmpty()) {
        asset.material = asset.materials[0];
        asset.baseColorTexture = asset.material.baseColor;
    }
    return asset;
}

void ImportNode(
        GameObject& object,
        const GltfSceneDocument& document,
        const std::uint32_t nodeIndex,
        const SceneMeshSlot slot,
        const Vector3& albedo,
        const char* gltfLibraryKey) {
    if (nodeIndex >= document.nodes.GetSize()) {
        return;
    }
    const GltfSceneNode& sceneNode = document.nodes[nodeIndex];

    if (!sceneNode.name.IsEmpty()) {
        object.GetName() = sceneNode.name;
    }
    ApplyLocalTransform(object, sceneNode.localTransform);

    if (sceneNode.HasMesh() && !sceneNode.hasSkin) {
        const GltfAsset assetView = MakeAssetView(document, sceneNode);
        GltfAssetBinder::BindRigidMesh(object, assetView, slot, albedo, gltfLibraryKey);
    }

    GameWorld& world = object.GetWorld();
    for (std::size_t childIndex = 0; childIndex < document.nodes.GetSize(); ++childIndex) {
        if (document.nodes[childIndex].parentIndex != static_cast<std::int32_t>(nodeIndex)) {
            continue;
        }
        GameObject* childObject = world.CreateGameObject();
        if (childObject == nullptr) {
            continue;
        }
        world.SetParent(childObject, &object);
        ImportNode(*childObject, document, static_cast<std::uint32_t>(childIndex), slot, albedo, gltfLibraryKey);
    }
}

bool CanApplySingleRootToOwner(const GameObject& owner, const GltfSceneDocument& document) noexcept {
    return document.rootNodeIndices.GetSize() == 1 && owner.GetComponent<MeshComponent>() == nullptr;
}

}  // namespace

bool GltfSceneImporter::ImportInto(
        GameObject& owner,
        const GltfSceneDocument& document,
        const SceneMeshSlot slot,
        const Vector3& albedo,
        const char* gltfLibraryKey) {
    if (document.IsEmpty()) {
        return false;
    }

    const char* libraryKey = gltfLibraryKey;
    if ((libraryKey == nullptr || libraryKey[0] == '\0') && !document.sourcePath.IsEmpty()) {
        libraryKey = document.sourcePath.CStr();
    }

    if (CanApplySingleRootToOwner(owner, document)) {
        const std::uint32_t rootIndex = document.rootNodeIndices[0];
        ImportNode(owner, document, rootIndex, slot, albedo, libraryKey);
        return true;
    }

    GameWorld& world = owner.GetWorld();
    for (std::size_t ri = 0; ri < document.rootNodeIndices.GetSize(); ++ri) {
        const std::uint32_t rootIndex = document.rootNodeIndices[ri];
        GameObject* rootObject = world.CreateGameObject();
        if (rootObject == nullptr) {
            continue;
        }
        world.SetParent(rootObject, &owner);
        ImportNode(*rootObject, document, rootIndex, slot, albedo, libraryKey);
    }
    return true;
}

}  // namespace Spark
