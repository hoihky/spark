#pragma once

#include "spark/character/ICharacterMotor.hpp"

namespace Spark {

class CharacterController3DComponent;
class TransformComponent;

/** Motor backed by <c>CharacterController3DComponent</c> on the body root. */
class KinematicCharacterMotor final : public ICharacterMotor {
public:
    void Bind(
            CharacterController3DComponent* controllerIn,
            TransformComponent* transformIn) noexcept;

    void SetGameplayMoveInput(const Vector3& worldVelocity) override;
    void ApplyWorldDelta(const Vector3& worldDelta) override;

    [[nodiscard]] CharacterLocomotionState QueryState() const override;
    [[nodiscard]] GameObject& GetBodyRoot() override;
    [[nodiscard]] const GameObject& GetBodyRoot() const override;

private:
    CharacterController3DComponent* controller = nullptr;
    TransformComponent* transform = nullptr;
};

}  // namespace Spark
