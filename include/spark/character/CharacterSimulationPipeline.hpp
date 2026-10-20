#pragma once

#include "spark/character/CharacterIntent.hpp"
#include "spark/physics/CharacterController3D.hpp"

namespace Spark {

class Character3DBodyComponent;
class FrameTiming;
class GameWorld;
class IEngineContext;
class PhysicsQueryWorld3D;

/**
 * Documented tick for controller-first 3D action characters (Souls-like).
 *
 * Shell integration:
 * 1. Demo <c>Simulate</c> — <c>RunPreAnimationControllerFirst</c> (intent → motor → controller sim → sync rig).
 * 2. <c>Game::OnUpdate</c> — components (FSM, animator, root motion into arbitrator).
 * 3. Body <c>Character3DBodyComponent</c> (priority 206) — <c>ApplyPostAnimationMotion</c> on the same frame.
 */
struct CharacterSimulationPipelineConfig {
    CharacterController3DSettings controllerSettings{};
};

struct CharacterSimulationPreAnimationParams {
    GameWorld& world;
    const FrameTiming& timing;
    Character3DBodyComponent& body;
    CharacterIntent intent;
    CharacterSimulationPipelineConfig config{};
    PhysicsQueryWorld3D* physicsQueries = nullptr;
};

void BindPhysicsQueriesForPresentation(GameWorld& world, PhysicsQueryWorld3D* queries) noexcept;

/** Intent + gameplay motor + character controller simulation (before component animation tick). */
void RunPreAnimationControllerFirst(const CharacterSimulationPreAnimationParams& params);

/** Applies arbitrator root motion after <c>RootMotionComponent</c> has sampled (same frame). */
void ApplyPostAnimationMotion(Character3DBodyComponent& body);

}  // namespace Spark
