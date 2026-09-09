#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class MaterialComponent;
class SkinnedMesh;
class Texture2D;
struct SceneDrawItem;

/**
 * Applies <c>MaterialComponent</c> / multi-material slots to skinned <c>SceneDrawItem</c> draws,
 * including base color, normal, metallic-roughness, and emissive texture layers.
 */
class SkinnedSceneDrawMaterialApplicator {
public:
    class TextureLayerResolver {
    public:
        TextureLayerResolver(SceneRenderParams& renderParams, const SceneSubmitDetail::FindSceneTextureFn& findTexture);

        [[nodiscard]] std::int32_t Resolve(
                const SharedPtr<Texture2D>& texture,
                Vector2* uvScale = nullptr,
                Vector2* uvOffset = nullptr) const;

        [[nodiscard]] const SceneSubmitDetail::FindSceneTextureFn& GetFindTexture() const noexcept {
            return findTexture;
        }

    private:
        SceneRenderParams& renderParams;
        const SceneSubmitDetail::FindSceneTextureFn& findTexture;
    };

    explicit SkinnedSceneDrawMaterialApplicator(
            SceneRenderParams& renderParams,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture);

    void ApplyMaterial(SceneDrawItem& item, const MaterialComponent& material);
    void ApplySlot(SceneDrawItem& item, const MultiMaterialComponent::Slot& slot);

    void AppendSkinnedDraws(
            Array<SceneDrawItem>& drawList,
            SceneDrawItem baseItem,
            const SkinnedMesh& mesh,
            const MaterialComponent* material,
            const MultiMaterialComponent* multiMaterial);

private:
    SceneRenderParams& renderParams;
    TextureLayerResolver textureResolver;
};

}  // namespace Spark
