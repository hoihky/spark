#include "spark/scene/detail/SceneSubmitLighting.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/lighting/DirectionalLightComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark::SceneSubmitDetail {

void ApplyEcsDirectionalLight(GameWorld& world, SceneRenderParams& params) {
    bool applied = false;
    world.ForEachGameObject([&](GameObject* object) {
        if (applied || object == nullptr) {
            return;
        }
        const DirectionalLightComponent* directional = object->GetComponent<DirectionalLightComponent>();
        if (directional == nullptr || !directional->IsEnabled()) {
            return;
        }
        const Matrix4 worldMatrix = object->GetWorldMatrix();
        Vector3 towardLight = worldMatrix.TransformVector(Vector3{0.0F, 0.0F, 1.0F});
        if (towardLight.LengthSquared() < 1.0e-10F) {
            towardLight = Vector3{0.0F, 1.0F, 0.0F};
        } else {
            towardLight = towardLight.Normalized();
        }
        params.lightDirectionWorld = towardLight;
        params.lightColor = directional->GetColor();
        params.lightIntensity = directional->GetIntensity();
        params.directionalShadowsEnabled = directional->CastsShadow();
        applied = true;
    });
}

}  // namespace Spark::SceneSubmitDetail
