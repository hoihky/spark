#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/SceneDrawableFrustumSink.hpp"
#include "spark/scene/submit/SkinnedSceneDrawMaterialApplicator.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class AnimatorComponent;
class GameObject;
class GameWorld;
class MaterialComponent;
class MeshComponent;
class SkinnedMeshComponent;
struct SceneDrawItem;

/** Rigid mesh frustum sink (sky, props, multi-submesh). */
class RigidDrawableSubmitCollector final : public DrawableFrustumSink {
public:
    RigidDrawableSubmitCollector(
            Array<SceneDrawItem>& drawList,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture,
            std::int32_t defaultShadowFlags,
            bool skiesSubmittedOutsideCull) noexcept;

    void OnDrawable(
            GameObject* object,
            const MeshComponent& mesh,
            const MaterialComponent* material,
            const Matrix4& worldMatrix) override;

private:
    Array<SceneDrawItem>& drawList;
    SceneRenderParams& params;
    SceneSubmitDetail::FindSceneTextureFn findTexture;
    std::int32_t defaultShadowFlags = 0;
    bool skiesSubmittedOutsideCull = false;
};

/** Skinned mesh frustum sink with joint palette resolve. */
class SkinnedDrawableSubmitCollector final : public SkinnedDrawableFrustumSink {
public:
    SkinnedDrawableSubmitCollector(
            GameWorld& world,
            Array<SceneDrawItem>& drawList,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findTexture,
            std::int32_t defaultShadowFlags) noexcept;

    void OnSkinnedDrawable(GameObject* object,
            const SkinnedMeshComponent& skinned,
            const MaterialComponent* material,
            const AnimatorComponent* animator,
            const Matrix4& worldMatrix) override;

private:
    static void FillJointPalette(
            GameObject* owner,
            const Matrix4& ownerWorld,
            GameWorld& gameWorld,
            const SkinnedMeshComponent& mesh,
            const AnimatorComponent* animator,
            Array<Matrix4>& outPalette);

    GameWorld& gameWorld;
    Array<SceneDrawItem>& drawList;
    SceneRenderParams& params;
    SkinnedSceneDrawMaterialApplicator materialApplicator;
    std::int32_t defaultShadowFlags = 0;
};

}  // namespace Spark