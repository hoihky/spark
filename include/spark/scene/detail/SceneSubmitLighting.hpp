#pragma once

namespace Spark {

class GameWorld;
struct SceneRenderParams;

namespace SceneSubmitDetail {

/** Applies the first enabled <c>DirectionalLightComponent</c> in the world to scene params. */
void ApplyEcsDirectionalLight(GameWorld& world, SceneRenderParams& params);

}  // namespace SceneSubmitDetail

}  // namespace Spark
