#pragma once

#include "spark/character/CharacterIntent.hpp"
#include "spark/character/KinematicCharacterMotor.hpp"
#include "spark/character/MotionArbitrationMode.hpp"
#include "spark/character/MotionArbitrator.hpp"
#include "spark/ecs/GameComponent.hpp"
namespace Spark {

class CharacterController3DComponent;
class GameObject;
class RootMotionComponent;
class TransformComponent;

/**
 * Physics root for a 3D action character: kinematic motor + motion arbitration.
 * Pair with <c>Character3DVisualComponent</c> on a child (skinned mesh + animator).
 */
class Character3DBodyComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Character3DBody;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 206; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetVisualObject(GameObject* object) noexcept { visualObject = object; }
    [[nodiscard]] GameObject* GetVisualObject() const noexcept { return visualObject; }

    void SetArbitrationMode(const MotionArbitrationMode mode) noexcept { arbitrator.SetMode(mode); }
    [[nodiscard]] MotionArbitrationMode GetArbitrationMode() const noexcept { return arbitrator.GetMode(); }

    void BeginSimulationFrame() noexcept { arbitrator.BeginFrame(); }

    [[nodiscard]] MotionArbitrator& GetArbitrator() noexcept { return arbitrator; }
    [[nodiscard]] const MotionArbitrator& GetArbitrator() const noexcept { return arbitrator; }

    [[nodiscard]] KinematicCharacterMotor& GetMotor() noexcept { return motor; }
    [[nodiscard]] const KinematicCharacterMotor& GetMotor() const noexcept { return motor; }

    /** Routes <c>RootMotionComponent</c> deltas through the arbitrator (call once after creating root motion). */
    void WireRootMotion(RootMotionComponent& rootMotion);

    /** Souls-like attack lunge: brief <c>RootMotionExclusive</c> while upper-body overlay plays. */
    void BeginAttackRootMotionWindow() noexcept;
    void EndAttackRootMotionWindow() noexcept;

    [[nodiscard]] bool ValidateHierarchy() const noexcept;

    /** Walks <c>from</c> and its parents for a <c>Character3DBodyComponent</c>. */
    [[nodiscard]] static Character3DBodyComponent* FindOnAncestors(GameObject& from) noexcept;

private:
    GameObject* visualObject = nullptr;
    MotionArbitrator arbitrator{};
    KinematicCharacterMotor motor{};
    CharacterController3DComponent* controller = nullptr;
    TransformComponent* bodyTransform = nullptr;
    RootMotionComponent* rootMotion = nullptr;
    MotionArbitrationMode savedArbitrationMode = MotionArbitrationMode::GameplayDriven;
    bool attackRootMotionWindowActive = false;
};

}  // namespace Spark
