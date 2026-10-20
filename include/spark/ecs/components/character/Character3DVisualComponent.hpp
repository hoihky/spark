#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

/** Skinned presentation child for <c>Character3DBodyComponent</c> (animator + optional IK). */
class Character3DVisualComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Character3DVisual;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetBodyObject(GameObject* object) noexcept { bodyObject = object; }
    [[nodiscard]] GameObject* GetBodyObject() const noexcept { return bodyObject; }

    [[nodiscard]] bool ValidateSetup() const noexcept;

private:
    GameObject* bodyObject = nullptr;
};

}  // namespace Spark
