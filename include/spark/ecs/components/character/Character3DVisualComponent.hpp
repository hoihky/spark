#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/** Skinned presentation child for <c>Character3DBodyComponent</c> (animator + optional IK). */
class Character3DVisualComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Character3DVisual;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(set_body_object)
    void SetBodyObject(GameObject* object) noexcept { bodyObject = object; }
    SPARK_SCRIPT_BIND(get_body_object)
    [[nodiscard]] GameObject* GetBodyObject() const noexcept { return bodyObject; }

    SPARK_SCRIPT_BIND(is_validate_setup)
    [[nodiscard]] bool ValidateSetup() const noexcept;

private:
    GameObject* bodyObject = nullptr;
};

}  // namespace Spark
