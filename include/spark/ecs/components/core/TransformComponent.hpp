#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/** Local TRS; changes emit SignalId::TransformChanged to sibling components. */
class TransformComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Transform;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_local_transform)
    [[nodiscard]] const Transform& GetLocalTransform() const noexcept { return local; }

    SPARK_SCRIPT_BIND(set_local_transform)
    void SetLocalTransform(const Transform& t);
    void SetTranslation(const Vector3& v);
    void SetRotation(const Quaternion& q);
    void SetScale(const Vector3& s);
    void SetUniformScale(float s);

private:
    void NotifyTransformChanged();

    Transform local;
};

}  // namespace Spark
