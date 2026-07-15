#include "spark/scene/serialization/ComponentSnapshotHandlersExtended.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/DirectionalLightComponent.hpp"
#include "spark/ecs/components/AnimatorComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/SceneSpatialPolicyComponent.hpp"
#include "spark/ecs/components/SkyComponent.hpp"
#include "spark/ecs/components/SpotLightComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/GameWorldAssetLoader.hpp"
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

Utf8String JoinAssetsRootPath(const char* assetsRoot, const char* relativePath) {
    Utf8String full(assetsRoot != nullptr ? assetsRoot : "");
    if (!full.IsEmpty()) {
        const std::size_t n = full.ByteLength();
        if (full.CStr()[n - 1] != '/') {
            full.AppendUtf8("/");
        }
    }
    if (relativePath != nullptr) {
        full.AppendUtf8(relativePath);
    }
    return full;
}

void DeferComponentRestore(
        GameObject& owner,
        const ComponentRecord& record,
        const SceneApplyContext& ctx) {
    if (ctx.onDeferredComponent != nullptr) {
        ctx.onDeferredComponent(&owner, record, ctx.deferredUserData);
    }
}

template<typename RequestFn>
bool TryDeferAsset(
        GameObject& owner,
        const ComponentRecord& record,
        GameWorld& world,
        const SceneApplyContext& ctx,
        const char* fullPath,
        AssetLoadJobKind kind,
        RequestFn&& request) {
    if (ctx.assetLoader == nullptr || fullPath == nullptr || fullPath[0] == '\0') {
        return false;
    }
    request();
    switch (kind) {
        case AssetLoadJobKind::Gltf:
            if (world.TryGetMeshByKeyOrPath(fullPath)) {
                return false;
            }
            break;
        case AssetLoadJobKind::SkinnedGltf:
            if (SkinnedGltfAsset tmp{}; world.TryGetCachedSkinnedGltf(fullPath, tmp)) {
                return false;
            }
            break;
        case AssetLoadJobKind::Texture:
            if (world.TryGetTextureByKeyOrPath(fullPath)) {
                return false;
            }
            break;
        case AssetLoadJobKind::MeshObj:
            if (world.TryGetMeshByKeyOrPath(fullPath)) {
                return false;
            }
            break;
    }
    const AssetLoadState st = ctx.assetLoader->GetState(fullPath, kind);
    if (st == AssetLoadState::Failed) {
        return false;
    }
    DeferComponentRestore(owner, record, ctx);
    return ctx.onDeferredComponent != nullptr;
}

class DirectionalLightSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::DirectionalLight; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "directional_light"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const DirectionalLightComponent* dl = owner.GetComponent<DirectionalLightComponent>();
        if (dl == nullptr) {
            return false;
        }
        const Vector3& c = dl->GetColor();
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %d %d",
                c.x,
                c.y,
                c.z,
                dl->GetIntensity(),
                dl->CastsShadow() ? 1 : 0,
                dl->IsEnabled() ? 1 : 0);
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
        Vector3 color{1.0F, 0.97F, 0.9F};
        float intensity = 0.92F;
        int shadow = 1;
        int enabled = 1;
        if (std::sscanf(record.payload.CStr(), "%f %f %f %f %d %d", &color.x, &color.y, &color.z, &intensity, &shadow, &enabled)
            < 4) {
            return false;
        }
        DirectionalLightComponent* dl = owner.GetComponent<DirectionalLightComponent>();
        if (dl == nullptr) {
            dl = owner.AddComponent<DirectionalLightComponent>(color, intensity);
        } else {
            dl->SetColor(color);
            dl->SetIntensity(intensity);
        }
        dl->SetCastsShadow(shadow != 0);
        dl->SetEnabled(enabled != 0);
        return true;
    }
};

class SpotLightSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SpotLight; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "spot_light"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SpotLightComponent* sl = owner.GetComponent<SpotLightComponent>();
        if (sl == nullptr) {
            return false;
        }
        const Vector3& c = sl->GetColor();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %d %d",
                c.x,
                c.y,
                c.z,
                sl->GetIntensity(),
                sl->GetRange(),
                sl->GetInnerConeDegrees(),
                sl->GetOuterConeDegrees(),
                sl->CastsShadow() ? 1 : 0,
                sl->IsEnabled() ? 1 : 0);
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
        Vector3 color{1.0F, 0.95F, 0.88F};
        float intensity = 4.0F;
        float range = 18.0F;
        float innerDeg = 28.0F;
        float outerDeg = 45.0F;
        int shadow = 0;
        int enabled = 1;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %f %f %d %d",
                    &color.x,
                    &color.y,
                    &color.z,
                    &intensity,
                    &range,
                    &innerDeg,
                    &outerDeg,
                    &shadow,
                    &enabled)
            < 7) {
            return false;
        }
        SpotLightComponent* sl = owner.GetComponent<SpotLightComponent>();
        if (sl == nullptr) {
            sl = owner.AddComponent<SpotLightComponent>(color, intensity, range, innerDeg, outerDeg);
        } else {
            sl->SetColor(color);
            sl->SetIntensity(intensity);
            sl->SetRange(range);
            sl->SetInnerConeDegrees(innerDeg);
            sl->SetOuterConeDegrees(outerDeg);
        }
        sl->SetCastsShadow(shadow != 0);
        sl->SetEnabled(enabled != 0);
        return true;
    }
};

class SkySnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Sky; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "sky"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const SkyComponent* sky = owner.GetComponent<SkyComponent>();
        if (sky == nullptr) {
            return false;
        }
        Utf8String texPath;
        if (ctx.resolveTexturePath != nullptr) {
            texPath = ctx.resolveTexturePath(owner, ctx.textureUserData);
        }
        const Vector3& tint = sky->GetTint();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %d %.6f %.6f %.6f \"%s\"",
                static_cast<unsigned>(sky->GetSkyMode()),
                sky->IsSkyEnabled() ? 1 : 0,
                tint.x,
                tint.y,
                tint.z,
                texPath.CStr());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
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
        unsigned mode = 0U;
        int enabled = 1;
        Vector3 tint{0.25F, 0.35F, 0.55F};
        char texPath[384]{};
        if (std::sscanf(record.payload.CStr(), "%u %d %f %f %f", &mode, &enabled, &tint.x, &tint.y, &tint.z) < 5) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        while (*cursor != '\0' && *cursor != '"') {
            ++cursor;
        }
        if (*cursor == '"') {
            ParseLeadingQuotedString(cursor, texPath, sizeof(texPath));
        }
        SkyComponent* sky = owner.GetComponent<SkyComponent>();
        if (sky == nullptr) {
            sky = owner.AddComponent<SkyComponent>(static_cast<SceneSkyMode>(mode));
        }
        sky->SetSkyMode(static_cast<SceneSkyMode>(mode));
        sky->SetSkyEnabled(enabled != 0);
        sky->SetTint(tint);
        if (texPath[0] != '\0' && ctx.assetsRoot != nullptr) {
            const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, texPath);
            if (TryDeferAsset(
                        owner,
                        record,
                        world,
                        ctx,
                        full.CStr(),
                        AssetLoadJobKind::Texture,
                        [&]() { ctx.assetLoader->RequestTexture(full.CStr()); })) {
                return true;
            }
            if (SharedPtr<Texture2D> tex = world.TryGetTextureByKeyOrPath(full.CStr())) {
                sky->SetSkyTexture(tex);
            } else if (SharedPtr<Texture2D> loaded = world.LoadTexture(full.CStr())) {
                sky->SetSkyTexture(loaded);
            }
        }
        return true;
    }
};

class SpriteSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Sprite; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "sprite"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const SpriteComponent* sp = owner.GetComponent<SpriteComponent>();
        if (sp == nullptr) {
            return false;
        }
        Utf8String texPath;
        if (ctx.resolveTexturePath != nullptr) {
            texPath = ctx.resolveTexturePath(owner, ctx.textureUserData);
        }
        const Vector4& tint = sp->GetTint();
        const Vector4& uv = sp->GetUvRect();
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %d",
                texPath.CStr(),
                tint.x,
                tint.y,
                tint.z,
                tint.w,
                uv.x,
                uv.y,
                uv.z,
                uv.w,
                sp->GetSortOrder());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
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
        char texPath[384]{};
        Vector4 tint{1.0F, 1.0F, 1.0F, 1.0F};
        Vector4 uv{0.0F, 0.0F, 1.0F, 1.0F};
        int sortOrder = 0;
        const char* cursor = record.payload.CStr();
        if (!ParseLeadingQuotedString(cursor, texPath, sizeof(texPath))) {
            return false;
        }
        if (std::sscanf(cursor, "%f %f %f %f %f %f %f %f %d", &tint.x, &tint.y, &tint.z, &tint.w, &uv.x, &uv.y, &uv.z, &uv.w, &sortOrder)
            < 8) {
            return false;
        }
        SharedPtr<Texture2D> texture;
        if (texPath[0] != '\0' && ctx.assetsRoot != nullptr) {
            const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, texPath);
            if (TryDeferAsset(
                        owner,
                        record,
                        world,
                        ctx,
                        full.CStr(),
                        AssetLoadJobKind::Texture,
                        [&]() { ctx.assetLoader->RequestTexture(full.CStr()); })) {
                return true;
            }
            texture = world.TryGetTextureByKeyOrPath(full.CStr());
            if (!texture) {
                texture = world.LoadTexture(full.CStr());
            }
        }
        if (!texture) {
            return false;
        }
        if (owner.GetComponent<SpriteComponent>() != nullptr) {
            return true;
        }
        owner.AddComponent<SpriteComponent>(texture, tint, uv, sortOrder);
        return true;
    }
};

class SceneSpatialPolicySnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SceneSpatialPolicy; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "spatial_policy"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const SceneSpatialPolicyComponent* pol = owner.GetComponent<SceneSpatialPolicyComponent>();
        if (pol == nullptr) {
            return false;
        }
        char buf[32]{};
        std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(pol->GetPartitionKind()));
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
        unsigned mode = 0U;
        if (std::sscanf(record.payload.CStr(), "%u", &mode) != 1) {
            return false;
        }
        if (owner.GetComponent<SceneSpatialPolicyComponent>() != nullptr) {
            return true;
        }
        owner.AddComponent<SceneSpatialPolicyComponent>(static_cast<ScenePartitionKind>(mode));
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

void RegisterExtendedSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<DirectionalLightSnapshotHandler>(registry);
    RegisterHandler<SpotLightSnapshotHandler>(registry);
    RegisterHandler<SkySnapshotHandler>(registry);
    RegisterHandler<SpriteSnapshotHandler>(registry);
    RegisterHandler<SceneSpatialPolicySnapshotHandler>(registry);
}

}  // namespace Spark
