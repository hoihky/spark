#pragma once

#include "spark/character/CharacterLocomotionState.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

/** Applies authoritative body motion for a 3D character (controller, rigidbody, or root transform). */
class ICharacterMotor {
public:
    virtual ~ICharacterMotor() = default;

    virtual void SetGameplayMoveInput(const Vector3& worldVelocity) = 0;
    virtual void ApplyWorldDelta(const Vector3& worldDelta) = 0;

    [[nodiscard]] virtual CharacterLocomotionState QueryState() const = 0;
    [[nodiscard]] virtual GameObject& GetBodyRoot() = 0;
    [[nodiscard]] virtual const GameObject& GetBodyRoot() const = 0;
};

}  // namespace Spark
