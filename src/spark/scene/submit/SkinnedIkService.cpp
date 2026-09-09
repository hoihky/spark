#include "spark/scene/submit/SkinnedIkService.hpp"

#include "spark/ecs/components/animation/AimIkComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/animation/FootIkComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

bool SkinnedIkService::ObjectUsesIk(const GameObject& owner) noexcept {
    const FootIkComponent* footIk = owner.GetComponent<FootIkComponent>();
    if (footIk != nullptr && footIk->IsEnabled()) {
        return true;
    }
    const AimIkComponent* aimIk = owner.GetComponent<AimIkComponent>();
    return aimIk != nullptr && aimIk->IsEnabled();
}

bool SkinnedIkService::ApplyToPose(
        GameObject& owner,
        const Matrix4& ownerWorld,
        const AnimatorComponent& animator,
        Array<Transform>& pose) {
    pipeline.SetGroundProbe(groundProbe);
    return pipeline.Apply(owner, ownerWorld, animator, pose);
}

}  // namespace Spark
