#pragma once

#include "spark/core/Function.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(set_prompt_text)
    void SetPromptText(const char* text) noexcept;
    SPARK_SCRIPT_BIND(get_prompt_text)
    [[nodiscard]] const char* GetPromptText() const noexcept { return promptText.CStr(); }

    SPARK_SCRIPT_BIND(set_interaction_radius)
    void SetInteractionRadius(const float radius) noexcept;
    SPARK_SCRIPT_BIND(get_interaction_radius)
    [[nodiscard]] float GetInteractionRadius() const noexcept { return interactionRadius; }

    /** When non-empty, only instigators with this tag may interact. */
    SPARK_SCRIPT_BIND(set_required_instigator_tag)
    void SetRequiredInstigatorTag(const char* tag) noexcept;
    SPARK_SCRIPT_BIND(get_required_instigator_tag)
    [[nodiscard]] const char* GetRequiredInstigatorTag() const noexcept { return requiredInstigatorTag.CStr(); }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetOnInteract(InteractCallback callback) { onInteract = MoveTemp(callback); }

    SPARK_SCRIPT_BIND(is_instigator_in_range)
    [[nodiscard]] bool IsInstigatorInRange(const GameObject& instigator) const noexcept;
    SPARK_SCRIPT_BIND(is_can_interact)
    [[nodiscard]] bool CanInteract(const GameObject& instigator) const noexcept;
    SPARK_SCRIPT_BIND(is_try_interact)
    bool TryInteract(GameObject& instigator) noexcept;

private:
    Utf8String promptText{"Interact"};
    Utf8String requiredInstigatorTag{};
    float interactionRadius = 1.25F;
    bool enabled = true;
    InteractCallback onInteract{};
};

}  // namespace Spark
