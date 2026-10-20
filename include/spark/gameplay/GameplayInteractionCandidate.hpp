#pragma once

namespace Spark {

class GameObject;
class InteractableComponent;

class GameplayInteractionCandidate final {
public:
    GameplayInteractionCandidate() noexcept = default;
    GameplayInteractionCandidate(InteractableComponent* interactableIn, GameObject* ownerIn, float distanceSquaredIn)
        : interactable(interactableIn), owner(ownerIn), distanceSquared(distanceSquaredIn) {}

    [[nodiscard]] InteractableComponent* GetInteractable() const noexcept { return interactable; }
    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }
    [[nodiscard]] float GetDistanceSquared() const noexcept { return distanceSquared; }

    [[nodiscard]] bool IsValid() const noexcept { return interactable != nullptr && owner != nullptr; }

private:
    InteractableComponent* interactable = nullptr;
    GameObject* owner = nullptr;
    float distanceSquared = 0.0F;
};

}  // namespace Spark
