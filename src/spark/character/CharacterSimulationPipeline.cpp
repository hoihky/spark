#include "spark/character/CharacterSimulationPipeline.hpp"

#include "spark/character/KinematicCharacterMotor.hpp"
#include "spark/character/MotionArbitrator.hpp"
#include "spark/ecs/components/character/Character3DBodyComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/CharacterController3D.hpp"
#include "spark/physics/PhysicsQueries3D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SkinnedIkService.hpp"

namespace Spark {

void BindPhysicsQueriesForPresentation(GameWorld& world, PhysicsQueryWorld3D* queries) noexcept {
    world.GetSkinnedIkService().SetPhysicsQueryWorld(queries);
}

void RunPreAnimationControllerFirst(const CharacterSimulationPreAnimationParams& params) {
    Character3DBodyComponent& body = params.body;
    body.BeginSimulationFrame();
    body.GetArbitrator().SetGameplayWishVelocity(params.intent.wishVelocityWorld);
    body.GetArbitrator().ApplyGameplayToMotor(body.GetMotor());

    SimulateCharacterControllers3D(params.world, params.timing, params.config.controllerSettings);

    if (params.physicsQueries != nullptr) {
        BindPhysicsQueriesForPresentation(params.world, params.physicsQueries);
    }
}

void ApplyPostAnimationMotion(Character3DBodyComponent& body) {
    body.GetArbitrator().ApplyRootMotionToMotor(body.GetMotor());
}

}  // namespace Spark
