#include "spark/scene/serialization/ComponentSnapshotHandlersGameplay.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/animation/AnimationEventVfxComponent.hpp"
#include "spark/ecs/components/animation/AttachmentSocketComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/audio/AudioListenerComponent.hpp"
#include "spark/ecs/components/camera/CameraFollow3DComponent.hpp"
#include "spark/ecs/components/camera/SpringArm3DComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/rendering/BillboardComponent.hpp"
#include "spark/ecs/components/rendering/DecalProjectorComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

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

class SpawnPointSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SpawnPoint; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "spawn_point"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SpawnPointComponent* sp = owner.GetComponent<SpawnPointComponent>();
        if (sp == nullptr) {
            return false;
        }
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" \"%s\"",
                sp->GetSpawnName().CStr(),
                sp->GetDefaultPrefabPath().CStr());
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
        char name[256]{};
        char prefab[256]{};
        if (!ParseLeadingQuotedString(cursor, name, sizeof(name))) {
            return false;
        }
        (void)ParseLeadingQuotedString(cursor, prefab, sizeof(prefab));
        SpawnPointComponent* sp = owner.GetComponent<SpawnPointComponent>();
        if (sp == nullptr) {
            sp = owner.AddComponent<SpawnPointComponent>();
        }
        sp->SetSpawnName(name);
        sp->SetDefaultPrefabPath(prefab);
        return true;
    }
};

class BoxCollider2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::BoxCollider2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "box_collider_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const BoxCollider2DComponent* box = owner.GetComponent<BoxCollider2DComponent>();
        if (box == nullptr) {
            return false;
        }
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %u %u %u",
                box->GetHalfExtents().x,
                box->GetHalfExtents().y,
                box->GetOffset().x,
                box->GetOffset().y,
                static_cast<unsigned>(box->GetCategoryBits()),
                static_cast<unsigned>(box->GetMaskBits()),
                box->GetIsTrigger() ? 1u : 0u);
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
        float hx = 0.5F;
        float hy = 0.5F;
        float ox = 0.0F;
        float oy = 0.0F;
        unsigned cat = 1;
        unsigned mask = 0xFFFF;
        unsigned trigger = 0;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %f %u %u %u", &hx, &hy, &ox, &oy, &cat, &mask, &trigger) < 7) {
            return false;
        }
        BoxCollider2DComponent* box = owner.GetComponent<BoxCollider2DComponent>();
        if (box == nullptr) {
            box = owner.AddComponent<BoxCollider2DComponent>();
        }
        box->SetHalfExtents({hx, hy});
        box->SetOffset({ox, oy});
        box->SetCategoryBits(static_cast<std::uint16_t>(cat));
        box->SetMaskBits(static_cast<std::uint16_t>(mask));
        box->SetIsTrigger(trigger != 0u);
        return true;
    }
};

class CircleCollider2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::CircleCollider2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "circle_collider_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const CircleCollider2DComponent* circle = owner.GetComponent<CircleCollider2DComponent>();
        if (circle == nullptr) {
            return false;
        }
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %u %u %u",
                circle->GetRadius(),
                circle->GetOffset().x,
                circle->GetOffset().y,
                static_cast<unsigned>(circle->GetCategoryBits()),
                static_cast<unsigned>(circle->GetMaskBits()),
                circle->GetIsTrigger() ? 1u : 0u);
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
        float ox = 0.0F;
        float oy = 0.0F;
        unsigned cat = 1;
        unsigned mask = 0xFFFF;
        unsigned trigger = 0;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %u %u %u", &radius, &ox, &oy, &cat, &mask, &trigger) < 6) {
            return false;
        }
        CircleCollider2DComponent* circle = owner.GetComponent<CircleCollider2DComponent>();
        if (circle == nullptr) {
            circle = owner.AddComponent<CircleCollider2DComponent>();
        }
        circle->SetRadius(radius);
        circle->SetOffset({ox, oy});
        circle->SetCategoryBits(static_cast<std::uint16_t>(cat));
        circle->SetMaskBits(static_cast<std::uint16_t>(mask));
        circle->SetIsTrigger(trigger != 0u);
        return true;
    }
};

class Rigidbody2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Rigidbody2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "rigidbody_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const Rigidbody2DComponent* rb = owner.GetComponent<Rigidbody2DComponent>();
        if (rb == nullptr) {
            return false;
        }
        char buf[96]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %.6f %.6f %.6f",
                static_cast<unsigned>(rb->GetBodyType()),
                rb->GetGravityScale(),
                rb->GetVelocity().x,
                rb->GetVelocity().y);
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
        unsigned bodyType = 2;
        float gravity = 1.0F;
        float vx = 0.0F;
        float vy = 0.0F;
        if (std::sscanf(record.payload.CStr(), "%u %f %f %f", &bodyType, &gravity, &vx, &vy) < 4) {
            return false;
        }
        Rigidbody2DComponent* rb = owner.GetComponent<Rigidbody2DComponent>();
        if (rb == nullptr) {
            rb = owner.AddComponent<Rigidbody2DComponent>();
        }
        rb->SetBodyType(static_cast<RigidbodyBodyType2D>(bodyType));
        rb->SetGravityScale(gravity);
        rb->SetVelocity({vx, vy});
        return true;
    }
};

class TilemapMapSourceSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TilemapMapSource; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "tilemap_map_source"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TilemapMapSourceComponent* src = owner.GetComponent<TilemapMapSourceComponent>();
        if (src == nullptr) {
            return false;
        }
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" \"%s\" %.6f %u %u",
                src->GetTmxPath().CStr(),
                src->GetSparkMapPath().CStr(),
                src->GetPixelsPerWorldUnit(),
                src->GetImportOnAttach() ? 1u : 0u,
                src->GetHotReload() ? 1u : 0u);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        char tmx[256]{};
        char sparkmap[256]{};
        float ppu = 32.0F;
        unsigned importOnAttach = 1;
        unsigned hotReload = 0;
        if (!ParseLeadingQuotedString(cursor, tmx, sizeof(tmx))) {
            return false;
        }
        (void)ParseLeadingQuotedString(cursor, sparkmap, sizeof(sparkmap));
        std::sscanf(cursor, "%f %u %u", &ppu, &importOnAttach, &hotReload);
        TilemapMapSourceComponent* src = owner.GetComponent<TilemapMapSourceComponent>();
        if (src == nullptr) {
            src = owner.AddComponent<TilemapMapSourceComponent>();
        }
        src->SetTmxPath(tmx);
        src->SetSparkMapPath(sparkmap);
        src->SetPixelsPerWorldUnit(ppu);
        src->SetImportOnAttach(importOnAttach != 0u);
        src->SetHotReload(hotReload != 0u);
        (void)src->ImportNow(owner, world);
        return true;
    }
};

class Character3DAnimFsmSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Character3DAnimFsm; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "character_3d_anim_fsm"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const Character3DAnimFsmComponent* fsm = owner.GetComponent<Character3DAnimFsmComponent>();
        if (fsm == nullptr) {
            return false;
        }
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "v2 %u %u %u %u %u %u %u %.6f %.6f %.6f %u %u %u %u %u %zu",
                fsm->GetIdleClipIndex(),
                fsm->GetWalkClipIndex(),
                fsm->GetRunClipIndex(),
                fsm->GetAttackClipIndex(),
                fsm->GetHurtClipIndex(),
                fsm->GetStaggerClipIndex(),
                fsm->GetDeathClipIndex(),
                fsm->GetWalkSpeedThreshold(),
                fsm->GetRunSpeedThreshold(),
                fsm->GetCrossfadeDuration(),
                fsm->IsLocomotionDrivingEnabled() ? 1u : 0u,
                fsm->IsManualClipActive() ? 1u : 0u,
                fsm->GetManualClipIndex(),
                static_cast<unsigned>(fsm->GetManualClipLoopMode()),
                fsm->IsLocomotionBlendEnabled() ? 1u : 0u,
                fsm->GetCombatBlackboardIntSlot());
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

        Character3DAnimFsmComponent* fsm = owner.GetComponent<Character3DAnimFsmComponent>();
        if (fsm == nullptr) {
            fsm = owner.AddComponent<Character3DAnimFsmComponent>();
        }

        if (std::strncmp(record.payload.CStr(), "v2 ", 3) == 0) {
            unsigned idle = 0;
            unsigned walk = 1;
            unsigned run = 0xFFFFFFFFu;
            unsigned attack = 0xFFFFFFFFu;
            unsigned hurt = 0xFFFFFFFFu;
            unsigned stagger = 0xFFFFFFFFu;
            unsigned death = 0xFFFFFFFFu;
            float walkThresh = 0.35F;
            float runThresh = 2.2F;
            float crossfade = 0.18F;
            unsigned locomotionDrive = 1;
            unsigned manualActive = 0;
            unsigned manualClip = 0;
            unsigned manualLoop = 0;
            unsigned blendEnabled = 1;
            std::size_t combatBbSlot = static_cast<std::size_t>(-1);
            if (std::sscanf(
                        record.payload.CStr() + 3,
                        "%u %u %u %u %u %u %u %f %f %f %u %u %u %u %u %zu",
                        &idle,
                        &walk,
                        &run,
                        &attack,
                        &hurt,
                        &stagger,
                        &death,
                        &walkThresh,
                        &runThresh,
                        &crossfade,
                        &locomotionDrive,
                        &manualActive,
                        &manualClip,
                        &manualLoop,
                        &blendEnabled,
                        &combatBbSlot) < 16) {
                return false;
            }
            fsm->SetLocomotionClips(idle, walk, run);
            fsm->SetCombatClips(attack, hurt, stagger, death);
            fsm->SetWalkSpeedThreshold(walkThresh);
            fsm->SetRunSpeedThreshold(runThresh);
            fsm->SetCrossfadeDuration(crossfade);
            fsm->SetLocomotionDrivingEnabled(locomotionDrive != 0u);
            fsm->SetLocomotionBlendEnabled(blendEnabled != 0u);
            fsm->SetCombatBlackboardIntSlot(combatBbSlot);
            if (manualActive != 0u) {
                fsm->SetManualClip(manualClip, static_cast<AnimLoopMode>(manualLoop));
            } else {
                fsm->ClearManualClip();
            }
            return true;
        }

        if (std::strncmp(record.payload.CStr(), "v1 ", 3) != 0) {
            return false;
        }
        unsigned idle = 0;
        unsigned walk = 1;
        unsigned run = 0xFFFFFFFFu;
        unsigned attack = 0xFFFFFFFFu;
        float walkThresh = 0.35F;
        float runThresh = 2.2F;
        float crossfade = 0.18F;
        unsigned locomotionDrive = 1;
        unsigned manualActive = 0;
        unsigned manualClip = 0;
        unsigned manualLoop = 0;
        if (std::sscanf(
                    record.payload.CStr() + 3,
                    "%u %u %u %u %f %f %f %u %u %u %u",
                    &idle,
                    &walk,
                    &run,
                    &attack,
                    &walkThresh,
                    &runThresh,
                    &crossfade,
                    &locomotionDrive,
                    &manualActive,
                    &manualClip,
                    &manualLoop) < 11) {
            return false;
        }
        fsm->SetLocomotionClips(idle, walk, run);
        fsm->SetAttackClip(attack);
        fsm->SetWalkSpeedThreshold(walkThresh);
        fsm->SetRunSpeedThreshold(runThresh);
        fsm->SetCrossfadeDuration(crossfade);
        fsm->SetLocomotionDrivingEnabled(locomotionDrive != 0u);
        if (manualActive != 0u) {
            fsm->SetManualClip(manualClip, static_cast<AnimLoopMode>(manualLoop));
        } else {
            fsm->ClearManualClip();
        }
        return true;
    }
};

class AttachmentSocketSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::AttachmentSocket; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "attachment_socket"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const AttachmentSocketComponent* sock = owner.GetComponent<AttachmentSocketComponent>();
        if (sock == nullptr) {
            return false;
        }
        const Vector3& off = sock->GetLocalOffset();
        const Quaternion& rot = sock->GetLocalRotation();
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %.6f %.6f %.6f %.6f %.6f %.6f %.6f %u",
                sock->GetJointIndex(),
                off.x,
                off.y,
                off.z,
                rot.x,
                rot.y,
                rot.z,
                rot.w,
                sock->IsEnabled() ? 1u : 0u);
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
        unsigned joint = 0;
        float ox = 0.0F;
        float oy = 0.0F;
        float oz = 0.0F;
        float qx = 0.0F;
        float qy = 0.0F;
        float qz = 0.0F;
        float qw = 1.0F;
        unsigned enabled = 1;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%u %f %f %f %f %f %f %f %u",
                    &joint,
                    &ox,
                    &oy,
                    &oz,
                    &qx,
                    &qy,
                    &qz,
                    &qw,
                    &enabled) < 9) {
            return false;
        }
        AttachmentSocketComponent* sock = owner.GetComponent<AttachmentSocketComponent>();
        if (sock == nullptr) {
            sock = owner.AddComponent<AttachmentSocketComponent>();
        }
        sock->SetJointIndex(joint);
        sock->SetLocalOffset({ox, oy, oz});
        sock->SetLocalRotation({qx, qy, qz, qw});
        sock->SetEnabled(enabled != 0u);
        return true;
    }
};

class AnimationEventReceiverSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::AnimationEventReceiver; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "animation_event_receiver"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const AnimationEventReceiverComponent* recv = owner.GetComponent<AnimationEventReceiverComponent>();
        if (recv == nullptr) {
            return false;
        }
        Utf8String payload;
        const Array<AnimationEventMarker>& markers = recv->GetMarkers();
        char countBuf[16]{};
        std::snprintf(countBuf, sizeof(countBuf), "%zu ", markers.GetSize());
        payload.AppendUtf8(countBuf);
        for (std::size_t i = 0; i < markers.GetSize(); ++i) {
            char buf[256]{};
            std::snprintf(
                    buf,
                    sizeof(buf),
                    "%u %.6f \"%s\" ",
                    markers[i].clipIndex,
                    markers[i].normalizedTime,
                    markers[i].eventName.CStr());
            payload.AppendUtf8(buf);
        }
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
        AnimationEventReceiverComponent* recv = owner.GetComponent<AnimationEventReceiverComponent>();
        if (recv == nullptr) {
            recv = owner.AddComponent<AnimationEventReceiverComponent>();
        }
        recv->ClearMarkers();
        std::size_t count = 0;
        const char* cursor = record.payload.CStr();
        if (std::sscanf(cursor, "%zu", &count) != 1) {
            return true;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        for (std::size_t i = 0; i < count; ++i) {
            unsigned clip = 0;
            float t = 0.0F;
            char name[128]{};
            if (std::sscanf(cursor, "%u %f", &clip, &t) < 2) {
                break;
            }
            while (*cursor != '\0' && *cursor != '"') {
                ++cursor;
            }
            if (!ParseLeadingQuotedString(cursor, name, sizeof(name))) {
                break;
            }
            recv->AddMarker(clip, t, name);
        }
        return true;
    }
};

class AnimationEventVfxSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::AnimationEventVfx; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "animation_event_vfx"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const AnimationEventVfxComponent* vfx = owner.GetComponent<AnimationEventVfxComponent>();
        if (vfx == nullptr) {
            return false;
        }
        const Array<AnimationEventVfxBinding>& bindings = vfx->GetBindings();
        const Vector3& offset = vfx->GetWorldOffset();
        Utf8String payload;
        char countBuf[16]{};
        std::snprintf(countBuf, sizeof(countBuf), "%zu ", bindings.GetSize());
        payload.AppendUtf8(countBuf);
        char offsetBuf[64]{};
        std::snprintf(offsetBuf, sizeof(offsetBuf), "%.6f %.6f %.6f ", offset.x, offset.y, offset.z);
        payload.AppendUtf8(offsetBuf);
        for (std::size_t i = 0; i < bindings.GetSize(); ++i) {
            char buf[384]{};
            std::snprintf(
                    buf,
                    sizeof(buf),
                    "\"%s\" \"%s\" ",
                    bindings[i].eventName.CStr(),
                    bindings[i].vfxAssetKey.CStr());
            payload.AppendUtf8(buf);
        }
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
        AnimationEventVfxComponent* vfx = owner.GetComponent<AnimationEventVfxComponent>();
        if (vfx == nullptr) {
            vfx = owner.AddComponent<AnimationEventVfxComponent>();
        }
        vfx->ClearBindings();
        std::size_t count = 0;
        Vector3 offset{};
        const char* cursor = record.payload.CStr();
        int consumed = 0;
        if (std::sscanf(cursor, "%zu %f %f %f %n", &count, &offset.x, &offset.y, &offset.z, &consumed) < 4) {
            return true;
        }
        vfx->SetWorldOffset(offset);
        cursor += consumed;
        for (std::size_t i = 0; i < count; ++i) {
            char eventName[128]{};
            char assetKey[128]{};
            if (!ParseLeadingQuotedString(cursor, eventName, sizeof(eventName))) {
                break;
            }
            if (!ParseLeadingQuotedString(cursor, assetKey, sizeof(assetKey))) {
                break;
            }
            vfx->AddBinding(eventName, assetKey);
        }
        return true;
    }
};

class DamageableSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Damageable; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "damageable"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const DamageableComponent* dmg = owner.GetComponent<DamageableComponent>();
        if (dmg == nullptr) {
            return false;
        }
        char buf[64]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %u",
                dmg->GetDamageMultiplier(),
                dmg->IsInvulnerable() ? 1u : 0u);
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
        float mult = 1.0F;
        unsigned invuln = 0;
        if (std::sscanf(record.payload.CStr(), "%f %u", &mult, &invuln) < 2) {
            return false;
        }
        DamageableComponent* dmg = owner.GetComponent<DamageableComponent>();
        if (dmg == nullptr) {
            dmg = owner.AddComponent<DamageableComponent>();
        }
        dmg->SetDamageMultiplier(mult);
        dmg->SetInvulnerable(invuln != 0u);
        return true;
    }
};

class BillboardSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Billboard; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "billboard"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const BillboardComponent* bb = owner.GetComponent<BillboardComponent>();
        if (bb == nullptr) {
            return false;
        }
        char buf[32]{};
        std::snprintf(buf, sizeof(buf), "%u %u", static_cast<unsigned>(bb->GetMode()), bb->IsEnabled() ? 1u : 0u);
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
        unsigned mode = 0;
        unsigned enabled = 1;
        if (std::sscanf(record.payload.CStr(), "%u %u", &mode, &enabled) < 2) {
            return false;
        }
        BillboardComponent* bb = owner.GetComponent<BillboardComponent>();
        if (bb == nullptr) {
            bb = owner.AddComponent<BillboardComponent>();
        }
        bb->SetMode(static_cast<BillboardMode>(mode));
        bb->SetEnabled(enabled != 0u);
        return true;
    }
};

class AudioListenerSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::AudioListener; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "audio_listener"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const AudioListenerComponent* listener = owner.GetComponent<AudioListenerComponent>();
        if (listener == nullptr) {
            return false;
        }
        char buf[32]{};
        std::snprintf(buf, sizeof(buf), "%d %u", listener->GetPriority(), listener->IsEnabled() ? 1u : 0u);
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
        int priority = 0;
        unsigned enabled = 1;
        if (std::sscanf(record.payload.CStr(), "%d %u", &priority, &enabled) < 2) {
            return false;
        }
        AudioListenerComponent* listener = owner.GetComponent<AudioListenerComponent>();
        if (listener == nullptr) {
            listener = owner.AddComponent<AudioListenerComponent>();
        }
        listener->SetPriority(priority);
        listener->SetEnabled(enabled != 0u);
        return true;
    }
};

class CameraFollow3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::CameraFollow3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "camera_follow_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const CameraFollow3DComponent* follow = owner.GetComponent<CameraFollow3DComponent>();
        if (follow == nullptr) {
            return false;
        }
        const Vector3& off = follow->GetTargetOffset();
        char buf[96]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %u",
                off.x,
                off.y,
                off.z,
                follow->GetFollowSmoothRate(),
                follow->GetLookAtTarget() ? 1u : 0u);
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
        float ox = 0.0F;
        float oy = 1.6F;
        float oz = 0.0F;
        float rate = 8.0F;
        unsigned look = 1;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %f %u", &ox, &oy, &oz, &rate, &look) < 5) {
            return false;
        }
        CameraFollow3DComponent* follow = owner.GetComponent<CameraFollow3DComponent>();
        if (follow == nullptr) {
            follow = owner.AddComponent<CameraFollow3DComponent>();
        }
        follow->SetTargetOffset({ox, oy, oz});
        follow->SetFollowSmoothRate(rate);
        follow->SetLookAtTarget(look != 0u);
        return true;
    }
};

class SpringArm3DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SpringArm3D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "spring_arm_3d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SpringArm3DComponent* arm = owner.GetComponent<SpringArm3DComponent>();
        if (arm == nullptr) {
            return false;
        }
        const Vector3& off = arm->GetSocketOffset();
        char buf[160]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
                off.x,
                off.y,
                off.z,
                arm->GetArmLength(),
                arm->GetYawRadians(),
                arm->GetPitchRadians(),
                arm->GetProbeRadius(),
                arm->GetMinArmLength());
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
        float ox = 0.0F;
        float oy = 1.5F;
        float oz = 0.0F;
        float len = 4.0F;
        float yaw = 0.0F;
        float pitch = -0.25F;
        float probe = 0.2F;
        float minLen = 0.75F;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %f %f %f",
                    &ox,
                    &oy,
                    &oz,
                    &len,
                    &yaw,
                    &pitch,
                    &probe,
                    &minLen) < 8) {
            return false;
        }
        SpringArm3DComponent* arm = owner.GetComponent<SpringArm3DComponent>();
        if (arm == nullptr) {
            arm = owner.AddComponent<SpringArm3DComponent>();
        }
        arm->SetSocketOffset({ox, oy, oz});
        arm->SetArmLength(len);
        arm->SetYawRadians(yaw);
        arm->SetPitchRadians(pitch);
        arm->SetProbeRadius(probe);
        arm->SetMinArmLength(minLen);
        return true;
    }
};

class DecalProjectorSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::DecalProjector; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "decal_projector"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const DecalProjectorComponent* decal = owner.GetComponent<DecalProjectorComponent>();
        if (decal == nullptr) {
            return false;
        }
        const Vector3& size = decal->GetSize();
        char buf[96]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %u",
                size.x,
                size.y,
                size.z,
                decal->GetOpacity(),
                decal->IsEnabled() ? 1u : 0u);
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
        float sx = 1.0F;
        float sy = 1.0F;
        float sz = 0.5F;
        float opacity = 1.0F;
        unsigned enabled = 1;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %f %u", &sx, &sy, &sz, &opacity, &enabled) < 5) {
            return false;
        }
        DecalProjectorComponent* decal = owner.GetComponent<DecalProjectorComponent>();
        if (decal == nullptr) {
            decal = owner.AddComponent<DecalProjectorComponent>();
        }
        decal->SetSize({sx, sy, sz});
        decal->SetOpacity(opacity);
        decal->SetEnabled(enabled != 0u);
        return true;
    }
};

class AiAgentSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::AiAgent; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "ai_agent"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const AiAgentComponent* agent = owner.GetComponent<AiAgentComponent>();
        if (agent == nullptr) {
            return false;
        }
        Utf8String payload;
        payload.AppendUtf8("v1 ");
        char header[256]{};
        std::snprintf(
                header,
                sizeof(header),
                "%u %u %.6f %u %u %llu %llu %llu %u %zu ",
                agent->IsEnabled() ? 1u : 0u,
                agent->IsFsmEnabled() ? 1u : 0u,
                agent->GetMaxSpeed(),
                static_cast<unsigned>(agent->GetSteeringPlane()),
                agent->IsGoapEnabled() ? 1u : 0u,
                static_cast<unsigned long long>(agent->GetGoapWorldBits()),
                static_cast<unsigned long long>(agent->GetGoapGoalMask()),
                static_cast<unsigned long long>(agent->GetGoapGoalValue()),
                agent->IsFuzzyEnabled() ? 1u : 0u,
                agent->GetGoapActions().GetSize());
        payload.AppendUtf8(header);
        for (std::size_t i = 0; i < agent->GetGoapActions().GetSize(); ++i) {
            const GoapActionSpec& action = agent->GetGoapActions()[i];
            char buf[192]{};
            std::snprintf(
                    buf,
                    sizeof(buf),
                    "%llu %llu %llu %llu %.6f %u ",
                    static_cast<unsigned long long>(action.preMask),
                    static_cast<unsigned long long>(action.preValue),
                    static_cast<unsigned long long>(action.effectSetMask),
                    static_cast<unsigned long long>(action.effectClearMask),
                    action.cost,
                    action.nameId);
            payload.AppendUtf8(buf);
        }
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
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
        const char* cursor = record.payload.CStr();
        if (std::strncmp(cursor, "v1 ", 3) != 0) {
            return false;
        }
        cursor += 3;
        unsigned enabled = 1;
        unsigned fsmEnabled = 0;
        float maxSpeed = 4.0F;
        unsigned plane = 0;
        unsigned goapEnabled = 0;
        unsigned long long goapWorldBits = 0;
        unsigned long long goapGoalMask = 0;
        unsigned long long goapGoalValue = 0;
        unsigned fuzzyEnabled = 0;
        std::size_t actionCount = 0;
        if (std::sscanf(
                    cursor,
                    "%u %u %f %u %u %llu %llu %llu %u %zu",
                    &enabled,
                    &fsmEnabled,
                    &maxSpeed,
                    &plane,
                    &goapEnabled,
                    &goapWorldBits,
                    &goapGoalMask,
                    &goapGoalValue,
                    &fuzzyEnabled,
                    &actionCount) < 10) {
            return false;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        for (int skip = 0; skip < 10; ++skip) {
            while (*cursor != '\0' && *cursor != ' ') {
                ++cursor;
            }
            while (*cursor == ' ') {
                ++cursor;
            }
        }
        AiAgentComponent* agent = owner.GetComponent<AiAgentComponent>();
        if (agent == nullptr) {
            agent = owner.AddComponent<AiAgentComponent>();
        }
        agent->SetEnabled(enabled != 0u);
        agent->SetFsmEnabled(fsmEnabled != 0u);
        agent->SetMaxSpeed(maxSpeed);
        agent->SetSteeringPlane(static_cast<AiSteeringPlane>(plane));
        agent->SetGoapEnabled(goapEnabled != 0u);
        agent->SetGoapWorldBits(static_cast<std::uint64_t>(goapWorldBits));
        agent->SetGoapGoal(static_cast<std::uint64_t>(goapGoalMask), static_cast<std::uint64_t>(goapGoalValue));
        agent->SetFuzzyEnabled(fuzzyEnabled != 0u);
        agent->GetGoapActions().Clear();
        for (std::size_t i = 0; i < actionCount; ++i) {
            unsigned long long preMask = 0;
            unsigned long long preValue = 0;
            unsigned long long effectSetMask = 0;
            unsigned long long effectClearMask = 0;
            float cost = 1.0F;
            unsigned nameId = 0;
            if (std::sscanf(
                        cursor,
                        "%llu %llu %llu %llu %f %u",
                        &preMask,
                        &preValue,
                        &effectSetMask,
                        &effectClearMask,
                        &cost,
                        &nameId) < 6) {
                break;
            }
            for (int skip = 0; skip < 6; ++skip) {
                while (*cursor != '\0' && *cursor != ' ') {
                    ++cursor;
                }
                while (*cursor == ' ') {
                    ++cursor;
                }
            }
            GoapActionSpec action{};
            action.preMask = static_cast<std::uint64_t>(preMask);
            action.preValue = static_cast<std::uint64_t>(preValue);
            action.effectSetMask = static_cast<std::uint64_t>(effectSetMask);
            action.effectClearMask = static_cast<std::uint64_t>(effectClearMask);
            action.cost = cost;
            action.nameId = nameId;
            agent->GetGoapActions().PushBack(action);
        }
        return true;
    }
};

class PerceptionSensorSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PerceptionSensor; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "perception_sensor"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const PerceptionSensorComponent* sensor = owner.GetComponent<PerceptionSensorComponent>();
        if (sensor == nullptr) {
            return false;
        }
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "v1 %u %.6f %.6f %.6f %u",
                sensor->IsEnabled() ? 1u : 0u,
                sensor->GetSightRadius(),
                sensor->GetHearingRadius(),
                sensor->GetSightFovDegrees(),
                static_cast<unsigned>(sensor->GetTargetCategoryMask()));
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
        if (std::strncmp(record.payload.CStr(), "v1 ", 3) != 0) {
            return false;
        }
        unsigned enabled = 1;
        float sightRadius = 12.0F;
        float hearingRadius = 8.0F;
        float sightFov = 120.0F;
        unsigned targetMask = 0xFFFF;
        if (std::sscanf(
                    record.payload.CStr() + 3,
                    "%u %f %f %f %u",
                    &enabled,
                    &sightRadius,
                    &hearingRadius,
                    &sightFov,
                    &targetMask) < 5) {
            return false;
        }
        PerceptionSensorComponent* sensor = owner.GetComponent<PerceptionSensorComponent>();
        if (sensor == nullptr) {
            sensor = owner.AddComponent<PerceptionSensorComponent>();
        }
        sensor->SetEnabled(enabled != 0u);
        sensor->SetSightRadius(sightRadius);
        sensor->SetHearingRadius(hearingRadius);
        sensor->SetSightFovDegrees(sightFov);
        sensor->SetTargetCategoryMask(static_cast<std::uint16_t>(targetMask));
        sensor->ClearDetected();
        return true;
    }
};

}  // namespace

void RegisterGameplaySnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<SpawnPointSnapshotHandler>(registry);
    RegisterHandler<BoxCollider2DSnapshotHandler>(registry);
    RegisterHandler<CircleCollider2DSnapshotHandler>(registry);
    RegisterHandler<Rigidbody2DSnapshotHandler>(registry);
    RegisterHandler<TilemapMapSourceSnapshotHandler>(registry);
    RegisterHandler<Character3DAnimFsmSnapshotHandler>(registry);
    RegisterHandler<AttachmentSocketSnapshotHandler>(registry);
    RegisterHandler<AnimationEventReceiverSnapshotHandler>(registry);
    RegisterHandler<AnimationEventVfxSnapshotHandler>(registry);
    RegisterHandler<DamageableSnapshotHandler>(registry);
    RegisterHandler<BillboardSnapshotHandler>(registry);
    RegisterHandler<AudioListenerSnapshotHandler>(registry);
    RegisterHandler<CameraFollow3DSnapshotHandler>(registry);
    RegisterHandler<SpringArm3DSnapshotHandler>(registry);
    RegisterHandler<DecalProjectorSnapshotHandler>(registry);
    RegisterHandler<AiAgentSnapshotHandler>(registry);
    RegisterHandler<PerceptionSensorSnapshotHandler>(registry);
}

}  // namespace Spark
