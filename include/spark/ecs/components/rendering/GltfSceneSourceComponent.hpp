#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * References a glTF scene asset for prefab expansion.
 * Serialized as <c>gltf_scene</c>; expanded at load time via <c>GltfSceneImporter</c>.
 */
class GltfSceneSourceComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GltfSceneSource;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_gltf_asset_rel)
    [[nodiscard]] const Utf8String& GetGltfAssetRel() const noexcept { return gltfAssetRel; }
    SPARK_SCRIPT_BIND(set_gltf_asset_rel)
    void SetGltfAssetRel(const char* path) noexcept { gltfAssetRel = Utf8String(path != nullptr ? path : ""); }

private:
    Utf8String gltfAssetRel{};
};

}  // namespace Spark
