#include "spark/scene/serialization/ComponentSnapshotHandlersMore.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

bool ParseLeadingQuotedString(const char*& cursor, char* out, std::size_t outCap) noexcept {
    if (outCap == 0) {
        return false;
    }
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    if (*cursor != '"') {
        return false;
    }
    ++cursor;
    std::size_t n = 0;
    while (*cursor != '\0' && *cursor != '"') {
        if (*cursor == '\\' && cursor[1] == '"') {
            if (n + 1 < outCap) {
                out[n++] = '"';
            }
            cursor += 2;
            continue;
        }
        if (n + 1 < outCap) {
            out[n++] = *cursor;
        }
        ++cursor;
    }
    if (*cursor != '"') {
        return false;
    }
    ++cursor;
    out[n] = '\0';
    return true;
}

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

class TextOverlaySnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TextOverlay; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "text_overlay"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TextOverlayComponent* text = owner.GetComponent<TextOverlayComponent>();
        if (text == nullptr) {
            return false;
        }
        const Vector3& c = text->GetColor();
        char buf[1024]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" %.6f %.6f %.6f %.6f %.6f %.6f %.6f %d",
                text->GetText().CStr(),
                text->GetScreenX(),
                text->GetScreenY(),
                text->GetFontSizePixels(),
                c.x,
                c.y,
                c.z,
                text->GetAlpha(),
                text->IsVisible() ? 1 : 0);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        char textBuf[512]{};
        if (!ParseLeadingQuotedString(cursor, textBuf, sizeof(textBuf))) {
            return false;
        }
        float screenX = 8.0F;
        float screenY = 8.0F;
        float fontSize = 18.0F;
        Vector3 color{1.0F, 1.0F, 1.0F};
        float alpha = 1.0F;
        int visible = 1;
        if (std::sscanf(
                    cursor,
                    "%f %f %f %f %f %f %f %d",
                    &screenX,
                    &screenY,
                    &fontSize,
                    &color.x,
                    &color.y,
                    &color.z,
                    &alpha,
                    &visible)
            < 7) {
            return false;
        }
        TextOverlayComponent* text = owner.GetComponent<TextOverlayComponent>();
        if (text == nullptr) {
            text = owner.AddComponent<TextOverlayComponent>();
        }
        text->SetText(Utf8String(textBuf));
        text->SetScreenPosition(screenX, screenY);
        text->SetFontSizePixels(fontSize);
        text->SetColor(color);
        text->SetAlpha(alpha);
        text->SetVisible(visible != 0);
        return true;
    }
};

class ParticleEmitterSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::ParticleEmitter; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "particle_emitter"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>();
        if (pe == nullptr) {
            return false;
        }
        const Vector4& cs = pe->GetColorStart();
        const Vector4& ce = pe->GetColorEnd();
        const Vector3& grav = pe->GetGravity();
        const Vector3& dir = pe->GetEmissionDirection();
        char buf[768]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%d %u %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
                pe->IsEmitterEnabled() ? 1 : 0,
                pe->GetMaxParticles(),
                pe->GetEmissionRate(),
                pe->GetLifetimeMin(),
                pe->GetLifetimeMax(),
                pe->GetStartSize(),
                pe->GetEndSize(),
                cs.x,
                cs.y,
                cs.z,
                cs.w,
                ce.x,
                ce.y,
                ce.z,
                ce.w,
                grav.x,
                grav.y,
                grav.z,
                dir.x,
                dir.y,
                dir.z,
                pe->GetSpreadAngleRadians(),
                pe->GetSpeedMin(),
                pe->GetSpeedMax());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        int enabled = 1;
        unsigned maxParticles = 512U;
        float emissionRate = 48.0F;
        float lifeMin = 0.8F;
        float lifeMax = 1.6F;
        float sizeStart = 0.14F;
        float sizeEnd = 0.02F;
        Vector4 colorStart{0.95F, 0.85F, 0.35F, 1.0F};
        Vector4 colorEnd{0.9F, 0.2F, 0.05F, 0.0F};
        Vector3 gravity{0.0F, -1.8F, 0.0F};
        Vector3 emissionDir{0.0F, 1.0F, 0.0F};
        float spread = 0.55F;
        float speedMin = 1.2F;
        float speedMax = 2.8F;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%d %u %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                    &enabled,
                    &maxParticles,
                    &emissionRate,
                    &lifeMin,
                    &lifeMax,
                    &sizeStart,
                    &sizeEnd,
                    &colorStart.x,
                    &colorStart.y,
                    &colorStart.z,
                    &colorStart.w,
                    &colorEnd.x,
                    &colorEnd.y,
                    &colorEnd.z,
                    &colorEnd.w,
                    &gravity.x,
                    &gravity.y,
                    &gravity.z,
                    &emissionDir.x,
                    &emissionDir.y,
                    &emissionDir.z,
                    &spread,
                    &speedMin,
                    &speedMax)
            < 22) {
            return false;
        }
        ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>();
        if (pe == nullptr) {
            pe = owner.AddComponent<ParticleEmitterComponent>();
        }
        pe->SetEmitterEnabled(enabled != 0);
        pe->SetMaxParticles(maxParticles);
        pe->SetEmissionRate(emissionRate);
        pe->SetLifetime(lifeMin, lifeMax);
        pe->SetStartEndSize(sizeStart, sizeEnd);
        pe->SetStartEndColor(colorStart, colorEnd);
        pe->SetGravity(gravity);
        pe->SetEmissionDirection(emissionDir);
        pe->SetSpreadAngleRadians(spread);
        pe->SetSpeedRange(speedMin, speedMax);
        return true;
    }
};

class TerrainSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Terrain; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "terrain"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TerrainComponent* terrain = owner.GetComponent<TerrainComponent>();
        if (terrain == nullptr) {
            return false;
        }
        const TerrainGeneratorSettings& s = terrain->GetSettings();
        const Vector3 albedo = owner.GetComponent<MeshComponent>() != nullptr
                ? owner.GetComponent<MeshComponent>()->GetAlbedo()
                : Vector3{0.42F, 0.55F, 0.36F};
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%d %d %.6f %.6f %.6f %.6f %d %.6f %.6f %u %.6f %.6f %.6f %.6f",
                s.subdivX,
                s.subdivZ,
                s.halfExtentX,
                s.halfExtentZ,
                s.heightScale,
                s.noiseScale,
                static_cast<int>(s.octaves),
                s.persistence,
                s.lacunarity,
                s.seed,
                s.worldUnitsPerTextureRepeat,
                albedo.x,
                albedo.y,
                albedo.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        TerrainGeneratorSettings settings{};
        Vector3 albedo{0.42F, 0.55F, 0.36F};
        int octaves = 6;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%d %d %f %f %f %f %d %f %f %u %f %f %f %f",
                    &settings.subdivX,
                    &settings.subdivZ,
                    &settings.halfExtentX,
                    &settings.halfExtentZ,
                    &settings.heightScale,
                    &settings.noiseScale,
                    &octaves,
                    &settings.persistence,
                    &settings.lacunarity,
                    &settings.seed,
                    &settings.worldUnitsPerTextureRepeat,
                    &albedo.x,
                    &albedo.y,
                    &albedo.z)
            < 13) {
            return false;
        }
        settings.octaves = octaves;
        if (owner.GetComponent<TerrainComponent>() != nullptr) {
            return true;
        }
        if (owner.GetComponent<TransformComponent>() == nullptr) {
            owner.AddComponent<TransformComponent>();
        }
        TerrainComponent* terrain = owner.AddComponent<TerrainComponent>(settings, albedo);
        terrain->ResetHeightsToProcedural(owner);
        return true;
    }
};

class BoxCollider3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::BoxCollider3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "box_collider_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const BoxCollider3DComponent* box = owner.GetComponent<BoxCollider3DComponent>();
        if (box == nullptr) {
            return false;
        }
        const Vector3& h = box->GetHalfExtents();
        const Vector3& o = box->GetOffset();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f",
                h.x,
                h.y,
                h.z,
                o.x,
                o.y,
                o.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        Vector3 half{0.5F, 0.5F, 0.5F};
        Vector3 offset{Vector3::Zero};
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %f",
                    &half.x,
                    &half.y,
                    &half.z,
                    &offset.x,
                    &offset.y,
                    &offset.z)
            != 6) {
            return false;
        }
        BoxCollider3DComponent* box = owner.GetComponent<BoxCollider3DComponent>();
        if (box == nullptr) {
            box = owner.AddComponent<BoxCollider3DComponent>(half, offset);
        } else {
            box->SetHalfExtents(half);
            box->SetOffset(offset);
        }
        return true;
    }
};

class SphereCollider3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SphereCollider3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "sphere_collider_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SphereCollider3DComponent* sphere = owner.GetComponent<SphereCollider3DComponent>();
        if (sphere == nullptr) {
            return false;
        }
        const Vector3& o = sphere->GetOffset();
        char buf[128]{};
        std::snprintf(buf, sizeof(buf), "%.6f %.6f %.6f %.6f", sphere->GetRadius(), o.x, o.y, o.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float radius = 0.5F;
        Vector3 offset{Vector3::Zero};
        if (std::sscanf(record.payload.CStr(), "%f %f %f %f", &radius, &offset.x, &offset.y, &offset.z) != 4) {
            return false;
        }
        SphereCollider3DComponent* sphere = owner.GetComponent<SphereCollider3DComponent>();
        if (sphere == nullptr) {
            sphere = owner.AddComponent<SphereCollider3DComponent>(radius, offset);
        } else {
            sphere->SetRadius(radius);
            sphere->SetOffset(offset);
        }
        return true;
    }
};

class CapsuleCollider3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::CapsuleCollider3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "capsule_collider_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const CapsuleCollider3DComponent* capsule = owner.GetComponent<CapsuleCollider3DComponent>();
        if (capsule == nullptr) {
            return false;
        }
        const Vector3& o = capsule->GetOffset();
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %u %.6f %.6f %.6f",
                capsule->GetRadius(),
                capsule->GetHeight(),
                static_cast<unsigned>(capsule->GetDirection()),
                o.x,
                o.y,
                o.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float radius = 0.5F;
        float height = 2.0F;
        unsigned direction = static_cast<unsigned>(CapsuleDirection3D::Y);
        Vector3 offset{Vector3::Zero};
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %u %f %f %f",
                    &radius,
                    &height,
                    &direction,
                    &offset.x,
                    &offset.y,
                    &offset.z)
            != 6) {
            return false;
        }
        CapsuleCollider3DComponent* capsule = owner.GetComponent<CapsuleCollider3DComponent>();
        if (capsule == nullptr) {
            capsule = owner.AddComponent<CapsuleCollider3DComponent>(
                    radius,
                    height,
                    static_cast<CapsuleDirection3D>(std::min(direction, 2U)),
                    offset);
        } else {
            capsule->SetRadius(radius);
            capsule->SetHeight(height);
            capsule->SetDirection(static_cast<CapsuleDirection3D>(std::min(direction, 2U)));
            capsule->SetOffset(offset);
        }
        return true;
    }
};

class Rigidbody3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Rigidbody3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "rigidbody_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const Rigidbody3DComponent* rb = owner.GetComponent<Rigidbody3DComponent>();
        if (rb == nullptr) {
            return false;
        }
        const Vector3& vel = rb->GetVelocity();
        const Vector3& angVel = rb->GetAngularVelocity();
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
                static_cast<unsigned>(rb->GetBodyType()),
                rb->GetGravityScale(),
                rb->GetRestitution(),
                rb->GetLinearDamping(),
                rb->GetAngularDamping(),
                rb->GetInverseMass(),
                rb->GetInverseInertiaTensorScale(),
                vel.x,
                vel.y,
                vel.z,
                angVel.x,
                angVel.y,
                angVel.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        unsigned bodyType = 0U;
        float gravityScale = 1.0F;
        float restitution = 0.0F;
        float linearDamping = 0.0F;
        float angularDamping = 0.0F;
        float inverseMass = 1.0F;
        float inverseInertia = 0.0F;
        Vector3 velocity{Vector3::Zero};
        Vector3 angularVelocity{Vector3::Zero};
        if (std::sscanf(
                    record.payload.CStr(),
                    "%u %f %f %f %f %f %f %f %f %f %f %f %f",
                    &bodyType,
                    &gravityScale,
                    &restitution,
                    &linearDamping,
                    &angularDamping,
                    &inverseMass,
                    &inverseInertia,
                    &velocity.x,
                    &velocity.y,
                    &velocity.z,
                    &angularVelocity.x,
                    &angularVelocity.y,
                    &angularVelocity.z)
            < 7) {
            return false;
        }
        Rigidbody3DComponent* rb = owner.GetComponent<Rigidbody3DComponent>();
        if (rb == nullptr) {
            rb = owner.AddComponent<Rigidbody3DComponent>(
                    static_cast<RigidbodyBodyType3D>(bodyType), gravityScale);
        }
        rb->SetBodyType(static_cast<RigidbodyBodyType3D>(bodyType));
        rb->SetGravityScale(gravityScale);
        rb->SetRestitution(restitution);
        rb->SetLinearDamping(linearDamping);
        rb->SetAngularDamping(angularDamping);
        rb->SetInverseMass(inverseMass);
        rb->SetInverseInertiaTensorScale(inverseInertia);
        rb->SetVelocity(velocity);
        rb->SetAngularVelocity(angularVelocity);
        return true;
    }
};

class CharacterController3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::CharacterController3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "character_controller_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const CharacterController3DComponent* cc = owner.GetComponent<CharacterController3DComponent>();
        if (cc == nullptr) {
            return false;
        }
        const Vector3& o = cc->GetCenterOffset();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
                cc->GetRadius(),
                o.x,
                o.y,
                o.z,
                cc->GetSlopeLimitDegrees(),
                cc->GetStepOffset(),
                cc->GetSkinWidth(),
                cc->GetGravityScale());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float radius = 0.4F;
        Vector3 offset{0.0F, 0.4F, 0.0F};
        float slopeLimit = 50.0F;
        float stepOffset = 0.35F;
        float skinWidth = 0.02F;
        float gravityScale = 1.0F;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %f %f %f",
                    &radius,
                    &offset.x,
                    &offset.y,
                    &offset.z,
                    &slopeLimit,
                    &stepOffset,
                    &skinWidth,
                    &gravityScale)
            != 8) {
            return false;
        }
        CharacterController3DComponent* cc = owner.GetComponent<CharacterController3DComponent>();
        if (cc == nullptr) {
            cc = owner.AddComponent<CharacterController3DComponent>(radius, offset);
        }
        cc->SetRadius(radius);
        cc->SetCenterOffset(offset);
        cc->SetSlopeLimitDegrees(slopeLimit);
        cc->SetStepOffset(stepOffset);
        cc->SetSkinWidth(skinWidth);
        cc->SetGravityScale(gravityScale);
        return true;
    }
};

class TriggerVolume3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TriggerVolume3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "trigger_volume_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TriggerVolume3DComponent* volume = owner.GetComponent<TriggerVolume3DComponent>();
        if (volume == nullptr) {
            return false;
        }
        const Vector3& he = volume->GetHalfExtents();
        const Vector3& o = volume->GetOffset();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %.6f %.6f %.6f %.6f %.6f %u %.6f %.6f %.6f",
                static_cast<unsigned>(volume->GetShape()),
                he.x,
                he.y,
                he.z,
                volume->GetRadius(),
                volume->GetHeight(),
                static_cast<unsigned>(volume->GetCapsuleDirection()),
                o.x,
                o.y,
                o.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        unsigned shape = static_cast<unsigned>(TriggerVolume3DShape::Box);
        Vector3 half{0.5F, 0.5F, 0.5F};
        float radius = 0.5F;
        float height = 2.0F;
        unsigned direction = static_cast<unsigned>(CapsuleDirection3D::Y);
        Vector3 offset{Vector3::Zero};
        if (std::sscanf(
                    record.payload.CStr(),
                    "%u %f %f %f %f %f %u %f %f %f",
                    &shape,
                    &half.x,
                    &half.y,
                    &half.z,
                    &radius,
                    &height,
                    &direction,
                    &offset.x,
                    &offset.y,
                    &offset.z)
            != 10) {
            return false;
        }
        TriggerVolume3DComponent* volume = owner.GetComponent<TriggerVolume3DComponent>();
        if (volume == nullptr) {
            volume = owner.AddComponent<TriggerVolume3DComponent>(
                    static_cast<TriggerVolume3DShape>(std::min(shape, 2U)), half, offset);
        }
        volume->SetShape(static_cast<TriggerVolume3DShape>(std::min(shape, 2U)));
        volume->SetHalfExtents(half);
        volume->SetRadius(radius);
        volume->SetHeight(height);
        volume->SetCapsuleDirection(static_cast<CapsuleDirection3D>(std::min(direction, 2U)));
        volume->SetOffset(offset);
        return true;
    }
};

class PhysicsMaterial3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PhysicsMaterial3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "physics_material_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const PhysicsMaterial3DComponent* mat = owner.GetComponent<PhysicsMaterial3DComponent>();
        if (mat == nullptr) {
            return false;
        }
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f",
                mat->GetStaticFriction(),
                mat->GetDynamicFriction(),
                mat->GetRestitution());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float staticFriction = 0.55F;
        float dynamicFriction = 0.48F;
        float restitution = 0.22F;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f",
                    &staticFriction,
                    &dynamicFriction,
                    &restitution)
            != 3) {
            return false;
        }
        PhysicsMaterial3DComponent* mat = owner.GetComponent<PhysicsMaterial3DComponent>();
        if (mat == nullptr) {
            mat = owner.AddComponent<PhysicsMaterial3DComponent>(staticFriction, dynamicFriction, restitution);
        } else {
            mat->SetStaticFriction(staticFriction);
            mat->SetDynamicFriction(dynamicFriction);
            mat->SetRestitution(restitution);
        }
        return true;
    }
};

class HealthSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Health; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "health"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const HealthComponent* hp = owner.GetComponent<HealthComponent>();
        if (hp == nullptr) {
            return false;
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f %.6f", hp->GetMaximum(), hp->GetCurrent());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        float maxHp = 100.0F;
        float curHp = 100.0F;
        if (std::sscanf(record.payload.CStr(), "%f %f", &maxHp, &curHp) != 2) {
            return false;
        }
        HealthComponent* hp = owner.GetComponent<HealthComponent>();
        if (hp == nullptr) {
            hp = owner.AddComponent<HealthComponent>(maxHp);
        }
        hp->SetMaximum(maxHp);
        hp->SetCurrent(curHp);
        return true;
    }
};

class PolygonCollider2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PolygonCollider2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "polygon_collider_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const PolygonCollider2DComponent* poly = owner.GetComponent<PolygonCollider2DComponent>();
        if (poly == nullptr || poly->GetVertexCount() < 3) {
            return false;
        }
        Utf8String payload;
        for (std::uint32_t i = 0; i < poly->GetVertexCount(); ++i) {
            char buf[48];
            const Vector2& v = poly->GetVertices()[i];
            std::snprintf(buf, sizeof(buf), "%.6f %.6f ", v.x, v.y);
            payload.AppendUtf8(buf);
        }
        char tail[32];
        std::snprintf(
                tail,
                sizeof(tail),
                "|%u %u %u",
                static_cast<unsigned>(poly->GetCategoryBits()),
                static_cast<unsigned>(poly->GetMaskBits()),
                poly->GetIsTrigger() ? 1u : 0u);
        payload.AppendUtf8(tail);
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* sep = std::strchr(record.payload.CStr(), '|');
        if (sep == nullptr) {
            return false;
        }
        Array<Vector2> verts;
        const char* p = record.payload.CStr();
        while (p < sep) {
            float x = 0.0F;
            float y = 0.0F;
            if (std::sscanf(p, "%f %f", &x, &y) != 2) {
                break;
            }
            verts.PushBack(Vector2{x, y});
            while (p < sep && *p != ' ') {
                ++p;
            }
            while (p < sep && *p == ' ') {
                ++p;
            }
        }
        unsigned cat = 1u;
        unsigned mask = 0xFFFFu;
        unsigned trigger = 0u;
        std::sscanf(sep + 1, "%u %u %u", &cat, &mask, &trigger);
        PolygonCollider2DComponent* poly = owner.GetComponent<PolygonCollider2DComponent>();
        if (poly == nullptr) {
            poly = owner.AddComponent<PolygonCollider2DComponent>();
        }
        poly->SetVertices(verts);
        poly->SetCategoryBits(static_cast<std::uint16_t>(cat));
        poly->SetMaskBits(static_cast<std::uint16_t>(mask));
        poly->SetIsTrigger(trigger != 0u);
        return verts.GetSize() >= 3;
    }
};

}  // namespace

void RegisterMoreSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<TextOverlaySnapshotHandler>(registry);
    RegisterHandler<ParticleEmitterSnapshotHandler>(registry);
    RegisterHandler<TerrainSnapshotHandler>(registry);
    RegisterHandler<BoxCollider3DSnapshotHandler>(registry);
    RegisterHandler<SphereCollider3DSnapshotHandler>(registry);
    RegisterHandler<CapsuleCollider3DSnapshotHandler>(registry);
    RegisterHandler<Rigidbody3DSnapshotHandler>(registry);
    RegisterHandler<CharacterController3DSnapshotHandler>(registry);
    RegisterHandler<TriggerVolume3DSnapshotHandler>(registry);
    RegisterHandler<PhysicsMaterial3DSnapshotHandler>(registry);
    RegisterHandler<HealthSnapshotHandler>(registry);
    RegisterHandler<PolygonCollider2DSnapshotHandler>(registry);
}

}  // namespace Spark
