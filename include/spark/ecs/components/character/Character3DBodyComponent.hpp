#pragma once

#include "spark/character/CharacterIntent.hpp"
#include "spark/character/KinematicCharacterMotor.hpp"
#include "spark/character/MotionArbitrationMode.hpp"
#include "spark/character/MotionArbitrator.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"
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

    SPARK_SCRIPT_BIND(set_visual_object)
    void SetVisualObject(GameObject* object) noexcept { visualObject = object; }
    SPARK_SCRIPT_BIND(get_visual_object)
    [[nodiscard]] GameObject* GetVisualObject() const noexcept { return visualObject; }

    SPARK_SCRIPT_BIND(set_arbitration_mode)
    void SetArbitrationMode(const MotionArbitrationMode mode) noexcept { arbitrator.SetMode(mode); }
    SPARK_SCRIPT_BIND(get_arbitration_mode)
    [[nodiscard]] MotionArbitrationMode GetArbitrationMode() const noexcept { return arbitrator.GetMode(); }

    SPARK_SCRIPT_BIND(begin_simulation_frame)
    void BeginSimulationFrame() noexcept { arbitrator.BeginFrame(); }

    SPARK_SCRIPT_BIND(get_arbitrator)
    [[nodiscard]] MotionArbitrator& GetArbitrator() noexcept { return arbitrator; }
    SPARK_SCRIPT_BIND(get_arbitrator)
    [[nodiscard]] const MotionArbitrator& GetArbitrator() const noexcept { return arbitrator; }

    SPARK_SCRIPT_BIND(get_motor)
    [[nodiscard]] KinematicCharacterMotor& GetMotor() noexcept { return motor; }
    SPARK_SCRIPT_BIND(get_motor)
    [[nodiscard]] const KinematicCharacterMotor& GetMotor() const noexcept { return motor; }

    /** Routes <c>RootMotionComponent</c> deltas through the arbitrator (call once after creating root motion). */
    SPARK_SCRIPT_BIND(wire_root_motion)
    void WireRootMotion(RootMotionComponent& rootMotion);

    /** Souls-like attack lunge: brief <c>RootMotionExclusive</c> while upper-body overlay plays. */
    SPARK_SCRIPT_BIND(begin_attack_root_motion_window)
    void BeginAttackRootMotionWindow() noexcept;
    SPARK_SCRIPT_BIND(end_attack_root_motion_window)
    void EndAttackRootMotionWindow() noexcept;

    SPARK_SCRIPT_BIND(is_validate_hierarchy)
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
