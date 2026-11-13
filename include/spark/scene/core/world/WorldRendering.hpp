#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/world/WorldView.hpp"
#include "spark/scene/submit/LitScene.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"

namespace Spark {

class IEngineContext;
struct SceneRenderParams;

/** Lit scene fill/submit on a world. */
class WorldRendering final : public WorldView {
public:
    using WorldView::WorldView;

    void FillLit(
            IEngineContext& context,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const LitSceneSubmitOptions& options,
            SceneRenderParams& outParams) {
        LitScene::Fill(world_, context, viewProjection, cameraPositionWorld, options, outParams);
    }

    void SubmitLit(
            IEngineContext& context,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const LitSceneSubmitOptions& options) {
        LitScene::Submit(world_, context, viewProjection, cameraPositionWorld, options);
    }

    [[nodiscard]] bool SubmitLitWithCamera(IEngineContext& context, const LitSceneSubmitOptions& options) {
        return LitScene::SubmitWithCamera(world_, context, options);
    }

    [[nodiscard]] bool ResolveCamera(
            float framebufferWidth,
            float framebufferHeight,
            SceneRenderParams& outParams) const noexcept {
        return LitScene::TryResolveCamera(world_, framebufferWidth, framebufferHeight, outParams);
    }
};

}  // namespace Spark
