#include "spark/scene/Mesh.hpp"
#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/GltfRigidLoader.hpp"
#include "spark/scene/Texture2D.hpp"

#include "spark/memory/SharedPtr.hpp"

namespace Spark {

bool Mesh::TryLoadFromGltf(
        const char* path,
        Mesh& outMesh,
        SharedPtr<Texture2D>* outBaseColor,
        GltfMaterialDesc* outMaterial,
        Array<GltfMaterialDesc>* outMaterials) {
    GltfRigidLoadResult result{};
    if (!GltfRigidLoader{}.LoadFromFile(path, result) || !result.mesh) {
        return false;
    }
    outMesh = *result.mesh;
    if (outMaterials != nullptr) {
        *outMaterials = result.materials;
    }
    if (outMaterial != nullptr || outBaseColor != nullptr) {
        const GltfMaterialDesc& primary =
                result.materials.IsEmpty() ? GltfMaterialDesc{} : result.materials[0];
        if (outMaterial != nullptr) {
            *outMaterial = primary;
        }
        if (outBaseColor != nullptr) {
            *outBaseColor = primary.baseColor;
        }
    }
    return true;
}

}  // namespace Spark
