#include "spark/scene/serialization/ComponentSnapshotHandlersTilemap.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/TilemapComponentSnapshot.hpp"

#include <cstdio>

namespace Spark {

namespace {

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

class TilemapSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Tilemap; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "tilemap"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
        if (tilemap == nullptr) {
            return false;
        }
        Utf8String payload;
        if (!TilemapComponentSnapshot::TryCapture(*tilemap, owner, ctx, payload)) {
            return false;
        }
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) const override {
        if (!ComponentSnapshotPayload::KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        return TilemapComponentSnapshot::TryRestore(owner, record.payload.CStr(), world, ctx);
    }
};

class TilemapCollider2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TilemapCollider2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "tilemap_collider_2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TilemapCollider2DComponent* collider = owner.GetComponent<TilemapCollider2DComponent>();
        if (collider == nullptr) {
            return false;
        }
        char buf[64]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %u %u",
                static_cast<unsigned>(collider->GetCategoryBits()),
                static_cast<unsigned>(collider->GetMaskBits()),
                collider->GetIsTrigger() ? 1u : 0u);
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
        unsigned cat = 1;
        unsigned mask = 0xFFFF;
        unsigned trigger = 0;
        if (std::sscanf(record.payload.CStr(), "%u %u %u", &cat, &mask, &trigger) < 3) {
            return false;
        }
        TilemapCollider2DComponent* collider = owner.GetComponent<TilemapCollider2DComponent>();
        if (collider == nullptr) {
            collider = owner.AddComponent<TilemapCollider2DComponent>();
        }
        collider->SetCategoryBits(static_cast<std::uint16_t>(cat));
        collider->SetMaskBits(static_cast<std::uint16_t>(mask));
        collider->SetIsTrigger(trigger != 0u);
        return true;
    }
};

class TilemapGameplayGridSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TilemapGameplayGrid; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "tilemap_gameplay_grid"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TilemapGameplayGridComponent* grid = owner.GetComponent<TilemapGameplayGridComponent>();
        if (grid == nullptr) {
            return false;
        }
        char buf[32]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %u",
                static_cast<unsigned>(grid->GetWalkRule()),
                grid->GetAutoRebake() ? 1u : 0u);
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
        unsigned walkRule = 0;
        unsigned autoRebake = 0;
        if (std::sscanf(record.payload.CStr(), "%u %u", &walkRule, &autoRebake) < 2) {
            return false;
        }
        TilemapGameplayGridComponent* grid = owner.GetComponent<TilemapGameplayGridComponent>();
        if (grid == nullptr) {
            grid = owner.AddComponent<TilemapGameplayGridComponent>();
        }
        grid->SetWalkRule(static_cast<TilemapGameplayWalkRule>(walkRule));
        grid->SetAutoRebake(autoRebake != 0u);
        grid->RequestRebake();
        return true;
    }
};

}  // namespace

void RegisterTilemapSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<TilemapSnapshotHandler>(registry);
    RegisterHandler<TilemapCollider2DSnapshotHandler>(registry);
    RegisterHandler<TilemapGameplayGridSnapshotHandler>(registry);
}

}  // namespace Spark
