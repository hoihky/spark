#include "spark/scene/serialization/ComponentSnapshotHandlersRendering.hpp"

#include "spark/core/Utility.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/rendering/RenderLayerComponent.hpp"
#include "spark/ecs/components/rendering/SortingGroupComponent.hpp"
#include "spark/ecs/components/rendering/GltfSceneSourceComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/RenderLayerRegistry.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include <cstdio>
#include <cstring>
#include <cstring>

namespace Spark {

namespace {

bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

bool ParseLeadingQuotedString(const char*& cursor, char* out, const std::size_t outCap) noexcept {
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

class GltfSceneSourceSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::GltfSceneSource; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "gltf_scene"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const GltfSceneSourceComponent* source = owner.GetComponent<GltfSceneSourceComponent>();
        if (source == nullptr || source->GetGltfAssetRel().IsEmpty()) {
            return false;
        }
        char buf[512]{};
        std::snprintf(buf, sizeof(buf), "\"%s\"", source->GetGltfAssetRel().CStr());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& ctx) const override {
        if (!KindTagEquals(record.kind, GetKindTag()) || ctx.assetsRoot == nullptr) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        char assetRel[384]{};
        if (!ParseLeadingQuotedString(cursor, assetRel, sizeof(assetRel)) || assetRel[0] == '\0') {
            return false;
        }

        Utf8String fullPath = ScenePathResolver::ResolveReadablePath(assetRel);
        if (fullPath.IsEmpty() && ctx.assetsRoot != nullptr) {
            char joined[1024]{};
            std::snprintf(joined, sizeof(joined), "%s/%s", ctx.assetsRoot, assetRel);
            fullPath = ScenePathResolver::ResolveReadablePath(joined);
        }
        if (fullPath.IsEmpty()) {
            return false;
        }
        if (!GltfAssetBinder::BindFromPathAsChild(owner, fullPath.CStr())) {
            return false;
        }
        GltfSceneSourceComponent* source = owner.GetComponent<GltfSceneSourceComponent>();
        if (source == nullptr) {
            source = owner.AddComponent<GltfSceneSourceComponent>();
        }
        source->SetGltfAssetRel(assetRel);
        return true;
    }
};

class RenderLayerSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::RenderLayer; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "RenderLayer"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const RenderLayerComponent* layer = owner.GetComponent<RenderLayerComponent>();
        if (layer == nullptr) {
            return false;
        }
        const RenderLayerRegistry& registry = RenderLayerRegistry::Instance();
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" %d %d",
                registry.GetLayerName(layer->GetLayerId()),
                layer->GetOrderInLayer(),
                layer->HasExplicitOrderInLayer() ? 1 : 0);
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
        char layerName[96]{};
        int orderInLayer = 0;
        int explicitOrder = 0;
        if (!ParseLeadingQuotedString(cursor, layerName, sizeof(layerName))) {
            return false;
        }
        if (std::sscanf(cursor, "%d %d", &orderInLayer, &explicitOrder) < 1) {
            return false;
        }
        RenderLayerRegistry& registry = RenderLayerRegistry::Instance();
        const RenderLayerId layerId = registry.FindLayerIdByName(layerName);
        if (layerId == kInvalidRenderLayerId) {
            return false;
        }
        RenderLayerComponent* layer = owner.GetComponent<RenderLayerComponent>();
        if (layer == nullptr) {
            layer = owner.AddComponent<RenderLayerComponent>(layerId);
        } else {
            layer->SetLayerId(layerId);
        }
        if (explicitOrder != 0) {
            layer->SetOrderInLayer(orderInLayer);
        } else {
            layer->ClearExplicitOrderInLayer();
        }
        return true;
    }
};

class SortingGroupSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SortingGroup; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "SortingGroup"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SortingGroupComponent* group = owner.GetComponent<SortingGroupComponent>();
        if (group == nullptr) {
            return false;
        }
        char buf[96]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%d %d %d",
                group->IsEnabled() ? 1 : 0,
                group->GetSortingOrder(),
                group->UsesRootWorldY() ? 1 : 0);
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
        int sortingOrder = 0;
        int sortAtRoot = 1;
        if (std::sscanf(record.payload.CStr(), "%d %d %d", &enabled, &sortingOrder, &sortAtRoot) < 2) {
            return false;
        }
        SortingGroupComponent* group = owner.GetComponent<SortingGroupComponent>();
        if (group == nullptr) {
            group = owner.AddComponent<SortingGroupComponent>(sortingOrder);
        }
        group->SetEnabled(enabled != 0);
        group->SetSortingOrder(sortingOrder);
        group->SetSortAtRootWorldY(sortAtRoot != 0);
        return true;
    }
};

class Camera2DSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Camera2D; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "camera2d"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const Camera2DComponent* cam = owner.GetComponent<Camera2DComponent>();
        if (cam == nullptr) {
            return false;
        }
        char buf[160]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %d %d",
                cam->GetHalfExtentY(),
                cam->GetClipNearZ(),
                cam->GetClipFarZ(),
                cam->GetPriority(),
                cam->IsEnabled() ? 1 : 0);
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
        float halfY = 5.0F;
        float nearZ = -500.0F;
        float farZ = 500.0F;
        int priority = 0;
        int enabled = 1;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %d %d", &halfY, &nearZ, &farZ, &priority, &enabled) < 5) {
            return false;
        }
        Camera2DComponent* cam = owner.GetComponent<Camera2DComponent>();
        if (cam == nullptr) {
            cam = owner.AddComponent<Camera2DComponent>();
        }
        cam->SetHalfExtentY(halfY);
        cam->SetClipNearZ(nearZ);
        cam->SetClipFarZ(farZ);
        cam->SetPriority(static_cast<std::int32_t>(priority));
        cam->SetEnabled(enabled != 0);
        return true;
    }
};

GameObject* FindGameObjectByName(GameWorld& world, const char* name) noexcept {
    if (name == nullptr || name[0] == '\0') {
        return nullptr;
    }
    GameObject* found = nullptr;
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr || found != nullptr) {
            return;
        }
        if (std::strcmp(o->GetName().CStr(), name) == 0) {
            found = o;
        }
    });
    return found;
}

class Camera2DRigSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Camera2DRig; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "camera2d_rig"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const Camera2DRigComponent* rig = owner.GetComponent<Camera2DRigComponent>();
        if (rig == nullptr) {
            return false;
        }
        const char* targetName = "";
        if (rig->GetTarget() != nullptr) {
            targetName = rig->GetTarget()->GetName().CStr();
        }
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u \"%.64s\" %.6f %.6f %.6f %.6f %.6f %d %.6f %.6f %.6f %.6f %d %.6f %.6f",
                static_cast<unsigned>(rig->GetMode()),
                targetName,
                rig->GetTargetOffset().x,
                rig->GetTargetOffset().y,
                rig->GetTargetOffset().z,
                rig->GetFollowSmoothRate(),
                rig->GetLookAheadScale(),
                rig->GetUseBounds() ? 1 : 0,
                rig->GetBoundsMin().x,
                rig->GetBoundsMin().y,
                rig->GetBoundsMax().x,
                rig->GetBoundsMax().y,
                rig->GetUseZoomLimits() ? 1 : 0,
                rig->GetZoomMinHalfExtentY(),
                rig->GetZoomMaxHalfExtentY());
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
        unsigned mode = 0U;
        char targetName[96]{};
        float offX = 0.0F;
        float offY = 0.0F;
        float offZ = 0.0F;
        float smooth = 7.5F;
        float lookAhead = 0.0F;
        int useBounds = 0;
        float bminX = 0.0F;
        float bminY = 0.0F;
        float bmaxX = 0.0F;
        float bmaxY = 0.0F;
        int useZoom = 0;
        float zoomMin = 2.0F;
        float zoomMax = 40.0F;
        const char* cursor = record.payload.CStr();
        if (std::sscanf(cursor, "%u", &mode) < 1) {
            return false;
        }
        while (*cursor != '\0' && *cursor != '"') {
            ++cursor;
        }
        if (!ParseLeadingQuotedString(cursor, targetName, sizeof(targetName))) {
            return false;
        }
        if (std::sscanf(
                    cursor,
                    "%f %f %f %f %f %d %f %f %f %f %d %f %f",
                    &offX,
                    &offY,
                    &offZ,
                    &smooth,
                    &lookAhead,
                    &useBounds,
                    &bminX,
                    &bminY,
                    &bmaxX,
                    &bmaxY,
                    &useZoom,
                    &zoomMin,
                    &zoomMax) < 14) {
            return false;
        }
        Camera2DRigComponent* rig = owner.GetComponent<Camera2DRigComponent>();
        if (rig == nullptr) {
            rig = owner.AddComponent<Camera2DRigComponent>();
        }
        rig->SetMode(static_cast<Camera2DRigMode>(mode));
        rig->SetTarget(FindGameObjectByName(world, targetName));
        rig->SetTargetOffset({offX, offY, offZ});
        rig->SetFollowSmoothRate(smooth);
        rig->SetLookAheadScale(lookAhead);
        rig->SetUseBounds(useBounds != 0);
        rig->SetBoundsMin({bminX, bminY});
        rig->SetBoundsMax({bmaxX, bmaxY});
        rig->SetUseZoomLimits(useZoom != 0);
        rig->SetZoomMinHalfExtentY(zoomMin);
        rig->SetZoomMaxHalfExtentY(zoomMax);
        return true;
    }
};

class MultiMaterialSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::MultiMaterial; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "multi_material"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const MultiMaterialComponent* multi = owner.GetComponent<MultiMaterialComponent>();
        if (multi == nullptr || multi->GetSlotCount() == 0) {
            return false;
        }
        bool anyLibrarySlot = false;
        for (std::size_t i = 0; i < multi->GetSlotCount(); ++i) {
            if (multi->SlotHasMaterialAsset(i)) {
                anyLibrarySlot = true;
                break;
            }
        }
        Utf8String payload;
        payload.AppendUtf8(anyLibrarySlot ? "v2 " : "v1 ");
        char countBuf[32]{};
        std::snprintf(countBuf, sizeof(countBuf), "%zu ", multi->GetSlotCount());
        payload.AppendUtf8(countBuf);
        for (std::size_t i = 0; i < multi->GetSlotCount(); ++i) {
            if (anyLibrarySlot) {
                payload.AppendUtf8("\"");
                payload.AppendUtf8(multi->GetSlotMaterialAssetKey(i).CStr());
                payload.AppendUtf8("\" ");
            }
            MaterialSlotSnapshot::Data slotData{};
            MaterialSlotSnapshot::CaptureFromSlot(multi->GetSlot(i), ctx, owner, slotData);
            MaterialSlotSnapshot::AppendSlotV1(slotData, payload);
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
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        if (std::strncmp(record.payload.CStr(), "v2 ", 3) == 0) {
            return TryRestoreV2(owner, record, world, ctx);
        }
        if (std::strncmp(record.payload.CStr(), "v1 ", 3) != 0) {
            return false;
        }
        return TryRestoreV1(owner, record, world, ctx);
    }

private:
    [[nodiscard]] static bool TryRestoreV1(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) {
        const char* cursor = record.payload.CStr() + 3;
        std::size_t slotCount = 0;
        if (std::sscanf(cursor, "%zu", &slotCount) != 1 || slotCount == 0) {
            return false;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        while (*cursor == ' ') {
            ++cursor;
        }

        MultiMaterialComponent* multi = owner.GetComponent<MultiMaterialComponent>();
        if (multi == nullptr) {
            multi = owner.AddComponent<MultiMaterialComponent>();
        }
        multi->ClearAllMaterialAssets(world);
        multi->Clear();
        multi->ResizeSlots(slotCount);

        for (std::size_t i = 0; i < slotCount; ++i) {
            MaterialSlotSnapshot::Data slotData{};
            if (!MaterialSlotSnapshot::TryParseSlotV1(cursor, slotData)) {
                return false;
            }
            MaterialSlotSnapshot::ApplyToSlot(multi->GetSlot(i), slotData, owner, world, ctx);
        }
        return true;
    }

    [[nodiscard]] static bool TryRestoreV2(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) {
        const char* cursor = record.payload.CStr() + 3;
        std::size_t slotCount = 0;
        if (std::sscanf(cursor, "%zu", &slotCount) != 1 || slotCount == 0) {
            return false;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        while (*cursor == ' ') {
            ++cursor;
        }

        MultiMaterialComponent* multi = owner.GetComponent<MultiMaterialComponent>();
        if (multi == nullptr) {
            multi = owner.AddComponent<MultiMaterialComponent>();
        }
        multi->ClearAllMaterialAssets(world);
        multi->Clear();
        multi->ResizeSlots(slotCount);

        for (std::size_t i = 0; i < slotCount; ++i) {
            char assetKey[512]{};
            if (!ParseLeadingQuotedString(cursor, assetKey, sizeof(assetKey))) {
                return false;
            }
            MaterialSlotSnapshot::Data slotData{};
            if (!MaterialSlotSnapshot::TryParseSlotV1(cursor, slotData)) {
                return false;
            }
            if (assetKey[0] != '\0') {
                multi->SetSlotMaterialAsset(world, i, assetKey);
                if (world.TryGetMaterialByKeyOrPath(assetKey) == nullptr && ctx.onDeferredComponent != nullptr) {
                    ctx.onDeferredComponent(&owner, record, ctx.deferredUserData);
                }
            }
            MaterialSlotSnapshot::ApplyToSlot(multi->GetSlot(i), slotData, owner, world, ctx);
        }
        return true;
    }
};

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

}  // namespace

void RegisterRenderingSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<GltfSceneSourceSnapshotHandler>(registry);
    RegisterHandler<RenderLayerSnapshotHandler>(registry);
    RegisterHandler<SortingGroupSnapshotHandler>(registry);
    RegisterHandler<Camera2DSnapshotHandler>(registry);
    RegisterHandler<Camera2DRigSnapshotHandler>(registry);
    RegisterHandler<MultiMaterialSnapshotHandler>(registry);
}

}  // namespace Spark
