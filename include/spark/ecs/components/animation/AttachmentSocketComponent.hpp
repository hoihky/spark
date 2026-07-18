#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/**
 * Positions an <c>attachedObject</c> at a skeleton joint each frame (weapon sockets, VFX anchors).
 * <c>sourceObject</c> defaults to the parent when null; must have an <c>AnimatorComponent</c>.
 */
class AttachmentSocketComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AttachmentSocket;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 250; }

    [[nodiscard]] GameObject* GetSourceObject() const noexcept { return sourceObject; }
    [[nodiscard]] GameObject* GetAttachedObject() const noexcept { return attachedObject; }
    [[nodiscard]] std::uint32_t GetJointIndex() const noexcept { return jointIndex; }
    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return localOffset; }
    [[nodiscard]] const Quaternion& GetLocalRotation() const noexcept { return localRotation; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetSourceObject(GameObject* o) noexcept { sourceObject = o; }
    void SetAttachedObject(GameObject* o) noexcept { attachedObject = o; }
    void SetJointIndex(std::uint32_t index) noexcept { jointIndex = index; }
    void SetLocalOffset(const Vector3& o) noexcept { localOffset = o; }
    void SetLocalRotation(const Quaternion& q) noexcept { localRotation = q; }
    void SetEnabled(bool e) noexcept { enabled = e; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    GameObject* sourceObject = nullptr;
    GameObject* attachedObject = nullptr;
    std::uint32_t jointIndex = 0;
    Vector3 localOffset{0.0F, 0.0F, 0.0F};
    Quaternion localRotation = Quaternion::Identity;
    bool enabled = true;
};

}  // namespace Spark
