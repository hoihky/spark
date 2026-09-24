#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/lighting/SceneLightingSetup.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
struct SceneRenderParams;

/**
 * Facade for the common lit-scene path: camera + lighting preset + ECS submit.
 *
 * Existing free functions (<c>FillStandardLitSceneFromWorld</c>, <c>SubmitStandardLitSceneFromWorld</c>)
 * are unchanged; this class composes them for readability and safer defaults.
 */
class LitSceneBuilder {
public:
    LitSceneBuilder(GameWorld& world, IEngineContext& context) noexcept;

    LitSceneBuilder& WithView(const Matrix4& viewProjection, const Vector3& cameraPositionWorld) noexcept;
    /** Resolves the main ECS camera using the current framebuffer size. Returns false when no camera exists. */
    [[nodiscard]] bool WithCameraFromWorld() noexcept;

    LitSceneBuilder& WithOptions(const LitSceneSubmitOptions& options) noexcept;
    LitSceneBuilder& WithLightingSetup(const SceneLightingSetup& setup) noexcept;

    /**
     * Fills <c>outParams</c> from ECS. Applies lighting setup first, then submit, then <c>Sanitize</c>.
     * Returns false when view/camera was not configured.
     */
    [[nodiscard]] bool Fill(SceneRenderParams& outParams) const noexcept;

    /** Fills and calls <c>IEngineContext::SetSceneRenderParams</c>. Returns false when fill fails. */
    [[nodiscard]] bool Submit() const noexcept;

private:
    GameWorld& world_;
    IEngineContext& context_;
    LitSceneSubmitOptions options_{};
    SceneLightingSetup lightingSetup_{SceneLightingSetup::FromProfile(SceneLightingProfile::Default)};
    bool hasLightingSetup_ = false;
    Matrix4 viewProjection_{};
    Vector3 cameraPositionWorld_{};
    bool hasView_ = false;
};

}  // namespace Spark
