#include "spark/scene/serialization/ComponentSnapshotHandlersPhysics2DExtended.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/HingeJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include <cstdio>

namespace Spark {

namespace {

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

class PhysicsMaterial2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PhysicsMaterial2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "physics_material_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const PhysicsMaterial2DComponent* material = owner.GetComponent<PhysicsMaterial2DComponent>();
        if (material == nullptr) {
            return false;
        }
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), "%.6f %.6f", material->GetDynamicFriction(), material->GetRestitution());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!ComponentSnapshotPayload::KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float friction = 0.48F;
        float restitution = 0.15F;
        if (std::sscanf(record.payload.CStr(), "%f %f", &friction, &restitution) < 2) {
            return false;
        }
        PhysicsMaterial2DComponent* material = owner.GetComponent<PhysicsMaterial2DComponent>();
        if (material == nullptr) {
            material = owner.AddComponent<PhysicsMaterial2DComponent>();
        }
        material->SetDynamicFriction(friction);
        material->SetRestitution(restitution);
        return true;
    }
};

class DistanceJoint2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::DistanceJoint2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "distance_joint_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const DistanceJoint2DComponent* joint = owner.GetComponent<DistanceJoint2DComponent>();
        if (joint == nullptr) {
            return false;
        }
        Utf8String payload;
        payload.AppendUtf8("v1 ");
        ComponentSnapshotPayload::AppendEntityRef(payload, joint->GetConnectedBody());
        const Vector2& anchorA = joint->GetLocalAnchorA();
        const Vector2& anchorB = joint->GetLocalAnchorB();
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f",
                joint->GetRestLength(),
                anchorA.x,
                anchorA.y,
                anchorB.x,
                anchorB.y,
                joint->GetStiffness());
        payload.AppendUtf8(buf);
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& ctx) const override {
        if (!ComponentSnapshotPayload::KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        if (std::strncmp(cursor, "v1 ", 3) != 0) {
            return false;
        }
        cursor += 3;
        std::uint64_t connectedId = 0;
        if (!ComponentSnapshotPayload::ParseEntityRef(cursor, connectedId)) {
            return false;
        }
        float restLength = 1.0F;
        float ax = 0.0F;
        float ay = 0.0F;
        float bx = 0.0F;
        float by = 0.0F;
        float stiffness = 0.55F;
        if (std::sscanf(cursor, "%f %f %f %f %f %f", &restLength, &ax, &ay, &bx, &by, &stiffness) < 6) {
            return false;
        }
        DistanceJoint2DComponent* joint = owner.GetComponent<DistanceJoint2DComponent>();
        if (joint == nullptr) {
            joint = owner.AddComponent<DistanceJoint2DComponent>();
        }
        joint->SetConnectedBody(ComponentSnapshotPayload::ResolveEntityRef(ctx, connectedId));
        joint->SetRestLength(restLength);
        joint->SetLocalAnchorA({ax, ay});
        joint->SetLocalAnchorB({bx, by});
        joint->SetStiffness(stiffness);
        return true;
    }
};

class HingeJoint2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::HingeJoint2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "hinge_joint_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const HingeJoint2DComponent* joint = owner.GetComponent<HingeJoint2DComponent>();
        if (joint == nullptr) {
            return false;
        }
        Utf8String payload;
        payload.AppendUtf8("v1 ");
        ComponentSnapshotPayload::AppendEntityRef(payload, joint->GetConnectedBody());
        const Vector2& anchorA = joint->GetLocalAnchorA();
        const Vector2& anchorB = joint->GetLocalAnchorB();
        char buf[96]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f",
                anchorA.x,
                anchorA.y,
                anchorB.x,
                anchorB.y,
                joint->GetStiffness());
        payload.AppendUtf8(buf);
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& ctx) const override {
        if (!ComponentSnapshotPayload::KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        if (std::strncmp(cursor, "v1 ", 3) != 0) {
            return false;
        }
        cursor += 3;
        std::uint64_t connectedId = 0;
        if (!ComponentSnapshotPayload::ParseEntityRef(cursor, connectedId)) {
            return false;
        }
        float ax = 0.0F;
        float ay = 0.0F;
        float bx = 0.0F;
        float by = 0.0F;
        float stiffness = 0.65F;
        if (std::sscanf(cursor, "%f %f %f %f %f", &ax, &ay, &bx, &by, &stiffness) < 5) {
            return false;
        }
        HingeJoint2DComponent* joint = owner.GetComponent<HingeJoint2DComponent>();
        if (joint == nullptr) {
            joint = owner.AddComponent<HingeJoint2DComponent>();
        }
        joint->SetConnectedBody(ComponentSnapshotPayload::ResolveEntityRef(ctx, connectedId));
        joint->SetLocalAnchorA({ax, ay});
        joint->SetLocalAnchorB({bx, by});
        joint->SetStiffness(stiffness);
        return true;
    }
};

}  // namespace

void RegisterPhysics2DExtendedSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<PhysicsMaterial2DSnapshotHandler>(registry);
    RegisterHandler<DistanceJoint2DSnapshotHandler>(registry);
    RegisterHandler<HingeJoint2DSnapshotHandler>(registry);
}

}  // namespace Spark
