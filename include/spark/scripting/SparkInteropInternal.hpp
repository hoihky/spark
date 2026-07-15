#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scripting/SparkInterop.h"

namespace Spark::Scripting {

[[nodiscard]] ComponentKind ToComponentKind(SparkComponentKind kind) noexcept;

template<typename T>
[[nodiscard]] T* AsComponent(SparkGameComponent* component, ComponentKind expected) noexcept {
    if (component == nullptr) {
        return nullptr;
    }
    auto* base = reinterpret_cast<GameComponent*>(component);
    if (base->Kind() != expected) {
        return nullptr;
    }
    return static_cast<T*>(base);
}

template<typename T>
[[nodiscard]] const T* AsComponent(const SparkGameComponent* component, ComponentKind expected) noexcept {
    return AsComponent<T>(const_cast<SparkGameComponent*>(component), expected);
}

[[nodiscard]] Vector3 ToVector3(SparkVector3 v) noexcept;
[[nodiscard]] SparkVector3 FromVector3(const Vector3& v) noexcept;
[[nodiscard]] Vector4 ToVector4(SparkVector4 v) noexcept;
[[nodiscard]] SparkVector4 FromVector4(const Vector4& v) noexcept;
[[nodiscard]] Vector2 ToVector2(SparkVector2 v) noexcept;
[[nodiscard]] SparkVector2 FromVector2(const Vector2& v) noexcept;
[[nodiscard]] Quaternion ToQuaternion(SparkQuaternion q) noexcept;
[[nodiscard]] SparkQuaternion FromQuaternion(const Quaternion& q) noexcept;
[[nodiscard]] Matrix4 ToMatrix4(const SparkMatrix4& m) noexcept;
[[nodiscard]] SparkMatrix4 FromMatrix4(const Matrix4& m) noexcept;

}  // namespace Spark::Scripting
