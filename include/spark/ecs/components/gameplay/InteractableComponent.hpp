#pragma once

#include "spark/core/Function.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

/**
 * Generic use / interact prompt. Pair with <c>TriggerVolume2DComponent</c> for overlap-based range,
 * or rely on <c>interactionRadius</c> around the owner's transform.
 */
class InteractableComponent final : public GameComponent {
public:
    using InteractCallback = Function<void(GameObject& instigator)>;

    static constexpr ComponentKind TypeKind = ComponentKind::Interactable;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetPromptText(const char* text) noexcept;
    [[nodiscard]] const char* GetPromptText() const noexcept { return promptText.CStr(); }

    void SetInteractionRadius(const float radius) noexcept;
    [[nodiscard]] float GetInteractionRadius() const noexcept { return interactionRadius; }

    /** When non-empty, only instigators with this tag may interact. */
    void SetRequiredInstigatorTag(const char* tag) noexcept;
    [[nodiscard]] const char* GetRequiredInstigatorTag() const noexcept { return requiredInstigatorTag.CStr(); }

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetOnInteract(InteractCallback callback) { onInteract = MoveTemp(callback); }

    [[nodiscard]] bool IsInstigatorInRange(const GameObject& instigator) const noexcept;
    [[nodiscard]] bool CanInteract(const GameObject& instigator) const noexcept;
    bool TryInteract(GameObject& instigator) noexcept;

private:
    Utf8String promptText{"Interact"};
    Utf8String requiredInstigatorTag{};
    float interactionRadius = 1.25F;
    bool enabled = true;
    InteractCallback onInteract{};
};

}  // namespace Spark
