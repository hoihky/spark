#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class MaterialComponent;
class Mesh;
class MultiMaterialComponent;
struct SceneDrawItem;

/** Expands multi-submesh rigid draws and resolves albedo textures into scene layers. */
class SceneRigidMeshDrawBuilder final {
public:
    SceneRigidMeshDrawBuilder(const SceneSubmitDetail::FindSceneTextureFn& findTexture, SceneRenderParams& params) noexcept;

    static void ApplyAlbedoTexture(
            SceneDrawItem& item,
            const SharedPtr<Texture2D>& baseColor,
            const Vector3& tint,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture);

    void AppendRigidMeshDraws(
            Array<SceneDrawItem>& drawList,
            SceneDrawItem baseItem,
            const Mesh& mesh,
            const MaterialComponent* mat,
            const MultiMaterialComponent* multiMat);

private:
    SceneSubmitDetail::FindSceneTextureFn findTexture;
    SceneRenderParams& params;
};

}  // namespace Spark