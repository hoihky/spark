#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"

#include "spark/scene/serialization/ComponentSnapshotHandlersExtended.hpp"
#include "spark/scene/serialization/ComponentSnapshotHandlersMore.hpp"
#include "spark/scene/serialization/ComponentSnapshotHandlersRendering.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/camera/CameraComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/core/Utility.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/GameWorldAssetLoader.hpp"
#include "spark/scene/GltfAssetBindings.hpp"
#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

/** Parses a leading `"path"` or `""` (empty paths are valid). */
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

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

class TransformSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Transform; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "transform"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TransformComponent* tr = owner.GetComponent<TransformComponent>();
        if (tr == nullptr) {
            return false;
        }
        const Transform& L = tr->GetLocalTransform();
        char buf[256]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
                L.scale.x,
                L.scale.y,
                L.scale.z,
                L.translation.x,
                L.translation.y,
                L.translation.z,
                L.rotation.x,
                L.rotation.y,
                L.rotation.z,
                L.rotation.w);
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
        Transform L{};
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %f %f %f %f %f",
                    &L.scale.x,
                    &L.scale.y,
                    &L.scale.z,
                    &L.translation.x,
                    &L.translation.y,
                    &L.translation.z,
                    &L.rotation.x,
                    &L.rotation.y,
                    &L.rotation.z,
                    &L.rotation.w)
            != 10) {
            return false;
        }
        TransformComponent* tr = owner.GetComponent<TransformComponent>();
        if (tr == nullptr) {
            tr = owner.AddComponent<TransformComponent>();
        }
        tr->SetLocalTransform(L);
        return true;
    }
};

const char* SlotToTag(SceneMeshSlot slot) noexcept {
    switch (slot) {
        case SceneMeshSlot::UnitCube:
            return "unit_cube";
        case SceneMeshSlot::GroundPlane:
            return "ground_plane";
        case SceneMeshSlot::Custom:
        default:
            return "custom";
    }
}

SceneMeshSlot TagToSlot(const char* tag) noexcept {
    if (tag == nullptr) {
        return SceneMeshSlot::Custom;
    }
    if (std::strcmp(tag, "unit_cube") == 0) {
        return SceneMeshSlot::UnitCube;
    }
    if (std::strcmp(tag, "ground_plane") == 0) {
        return SceneMeshSlot::GroundPlane;
    }
    return SceneMeshSlot::Custom;
}

class MeshSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Mesh; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "mesh"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        if (owner.GetComponent<PointLightComponent>() != nullptr) {
            return false;
        }
        const MeshComponent* mc = owner.GetComponent<MeshComponent>();
        if (mc == nullptr) {
            return false;
        }
        Utf8String asset;
        if (ctx.resolveMeshAssetPath != nullptr) {
            asset = ctx.resolveMeshAssetPath(owner, ctx.meshAssetUserData);
        }
        const Vector3& a = mc->GetAlbedo();
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%s \"%s\" %.6f %.6f %.6f",
                SlotToTag(mc->GetSlot()),
                asset.CStr(),
                a.x,
                a.y,
                a.z);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) const override {
        if (!KindTagEquals(record.kind, GetKindTag()) || ctx.assetsRoot == nullptr) {
            return false;
        }
        char slotTag[32]{};
        char asset[384]{};
        Vector3 albedo{1.0F, 1.0F, 1.0F};
        const char* cursor = record.payload.CStr();
        if (std::sscanf(cursor, "%31s", slotTag) != 1) {
            return false;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        while (*cursor == ' ') {
            ++cursor;
        }
        if (!ParseLeadingQuotedString(cursor, asset, sizeof(asset))) {
            return false;
        }
        if (std::sscanf(cursor, "%f %f %f", &albedo.x, &albedo.y, &albedo.z) < 3) {
            return false;
        }
        const SceneMeshSlot slot = TagToSlot(slotTag);
        SharedPtr<Mesh> mesh;
        if (std::strcmp(asset, "builtin:unit_cube") == 0) {
            mesh = world.TryGetMeshByKeyOrPath("spark/scene_editor/unit_cube");
            if (!mesh) {
                mesh = world.TryGetMeshByKeyOrPath("spark/demo/unit_cube");
            }
            if (!mesh) {
                auto m = MakeShared<Mesh>(Utf8String("UnitCube"));
                *m = Mesh::CreateUnitCube();
                mesh = world.RegisterMesh(m, "spark/scene/serialized_unit_cube");
            }
        } else if (asset[0] != '\0') {
            const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, asset);
            if (ctx.assetLoader != nullptr) {
                ctx.assetLoader->RequestGltf(full.CStr());
                GltfAsset gltfAsset{};
                if (world.TryGetCachedGltf(full.CStr(), gltfAsset)) {
                    mesh = gltfAsset.mesh;
                } else {
                    mesh = world.TryGetMeshByKeyOrPath(full.CStr());
                }
                if (!mesh) {
                    if (ctx.onDeferredComponent != nullptr) {
                        ctx.onDeferredComponent(&owner, record, ctx.deferredUserData);
                        return true;
                    }
                    return false;
                }
                if (owner.GetComponent<MeshComponent>() == nullptr) {
                    owner.AddComponent<MeshComponent>(mesh, slot, albedo);
                }
                if (world.TryGetCachedGltf(full.CStr(), gltfAsset) && gltfAsset.mesh) {
                    GltfAssetBinder::ApplyMaterials(owner, gltfAsset);
                }
                return true;
            } else {
                const GltfAsset g = world.LoadGltf(full.CStr());
                mesh = g.mesh;
                if (mesh) {
                    if (owner.GetComponent<MeshComponent>() == nullptr) {
                        owner.AddComponent<MeshComponent>(mesh, slot, albedo);
                    }
                    GltfAssetBinder::ApplyMaterials(owner, g);
                }
            }
        }
        if (!mesh) {
            return false;
        }
        if (owner.GetComponent<MeshComponent>() != nullptr) {
            return true;
        }
        owner.AddComponent<MeshComponent>(mesh, slot, albedo);
        return true;
    }
};

class MaterialSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Material; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "material"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        if (owner.GetComponent<PointLightComponent>() != nullptr) {
            return false;
        }
        const MaterialComponent* mat = owner.GetComponent<MaterialComponent>();
        if (mat == nullptr) {
            return false;
        }
        auto texPath = [&](const SharedPtr<Texture2D>& tex) -> Utf8String {
            if (tex) {
                return tex->GetName();
            }
            if (ctx.resolveTexturePath != nullptr) {
                return ctx.resolveTexturePath(owner, ctx.textureUserData);
            }
            return {};
        };
        const Vector3& tint = mat->GetTint();
        const Vector3& emissive = mat->GetEmissiveColor();
        char buf[2048]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "v2 \"%s\" \"%s\" \"%s\" \"%s\" %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f "
                "%.6f %.6f %.6f %d %.6f",
                texPath(mat->GetBaseColorTexture()).CStr(),
                texPath(mat->GetNormalTexture()).CStr(),
                texPath(mat->GetMetallicRoughnessTexture()).CStr(),
                texPath(mat->GetEmissiveTexture()).CStr(),
                tint.x,
                tint.y,
                tint.z,
                mat->GetMetallic(),
                mat->GetRoughness(),
                mat->GetMetallicFactor(),
                mat->GetRoughnessFactor(),
                mat->GetOcclusionStrength(),
                emissive.x,
                emissive.y,
                emissive.z,
                mat->GetEmissiveIntensity(),
                mat->IsDoubleSided() ? 1 : 0,
                mat->GetOpacity());
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
        if (std::strncmp(record.payload.CStr(), "v2 ", 3) == 0) {
            return TryRestoreV2(owner, record, world, ctx);
        }
        return TryRestoreLegacy(owner, record, world, ctx);
    }

private:
    [[nodiscard]] static bool TryRestoreLegacy(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) {
        char texPath[384]{};
        Vector3 tint{1.0F, 1.0F, 1.0F};
        float metallic = 0.0F;
        float roughness = 0.45F;
        const char* cursor = record.payload.CStr();
        if (!ParseLeadingQuotedString(cursor, texPath, sizeof(texPath))) {
            return false;
        }
        if (std::sscanf(cursor, "%f %f %f %f %f", &tint.x, &tint.y, &tint.z, &metallic, &roughness) < 5) {
            return false;
        }
        MaterialComponent* mat = owner.GetComponent<MaterialComponent>();
        if (mat == nullptr) {
            mat = owner.AddComponent<MaterialComponent>();
        }
        mat->SetTint(tint);
        mat->SetMetallic(metallic);
        mat->SetRoughness(roughness);
        BindMaterialTexture(owner, world, ctx, mat, texPath, &MaterialComponent::SetBaseColorTexture);
        return true;
    }

    [[nodiscard]] static bool TryRestoreV2(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) {
        const char* cursor = record.payload.CStr() + 3;
        char basePath[384]{};
        char normalPath[384]{};
        char mrPath[384]{};
        char emissivePath[384]{};
        if (!ParseLeadingQuotedString(cursor, basePath, sizeof(basePath)) ||
            !ParseLeadingQuotedString(cursor, normalPath, sizeof(normalPath)) ||
            !ParseLeadingQuotedString(cursor, mrPath, sizeof(mrPath)) ||
            !ParseLeadingQuotedString(cursor, emissivePath, sizeof(emissivePath))) {
            return false;
        }
        Vector3 tint{1.0F, 1.0F, 1.0F};
        float metallic = 0.0F;
        float roughness = 0.45F;
        float metallicFactor = 1.0F;
        float roughnessFactor = 1.0F;
        float occlusionStrength = 1.0F;
        Vector3 emissive{};
        float emissiveIntensity = 0.0F;
        int doubleSided = 0;
        float opacity = 1.0F;
        if (std::sscanf(
                    cursor,
                    "%f %f %f %f %f %f %f %f %f %f %f %f %f %d %f",
                    &tint.x,
                    &tint.y,
                    &tint.z,
                    &metallic,
                    &roughness,
                    &metallicFactor,
                    &roughnessFactor,
                    &occlusionStrength,
                    &emissive.x,
                    &emissive.y,
                    &emissive.z,
                    &emissiveIntensity,
                    &doubleSided,
                    &opacity) < 14) {
            return false;
        }
        MaterialComponent* mat = owner.GetComponent<MaterialComponent>();
        if (mat == nullptr) {
            mat = owner.AddComponent<MaterialComponent>();
        }
        mat->SetTint(tint);
        mat->SetMetallic(metallic);
        mat->SetRoughness(roughness);
        mat->SetMetallicFactor(metallicFactor);
        mat->SetRoughnessFactor(roughnessFactor);
        mat->SetOcclusionStrength(occlusionStrength);
        mat->SetEmissive(emissive, emissiveIntensity);
        mat->SetDoubleSided(doubleSided != 0);
        mat->SetOpacity(opacity);
        BindMaterialTexture(owner, world, ctx, mat, basePath, &MaterialComponent::SetBaseColorTexture);
        BindMaterialTexture(owner, world, ctx, mat, normalPath, &MaterialComponent::SetNormalTexture);
        BindMaterialTexture(owner, world, ctx, mat, mrPath, &MaterialComponent::SetMetallicRoughnessTexture);
        BindMaterialTexture(owner, world, ctx, mat, emissivePath, &MaterialComponent::SetEmissiveTexture);
        return true;
    }

    using TextureSetter = void (MaterialComponent::*)(SharedPtr<Texture2D>);

    static void BindMaterialTexture(
            GameObject& owner,
            GameWorld& world,
            const SceneApplyContext& ctx,
            MaterialComponent* mat,
            const char* texPath,
            TextureSetter setter) {
        if (mat == nullptr || texPath == nullptr || texPath[0] == '\0') {
            return;
        }
        auto tryBind = [&](const char* key) -> bool {
            if (key == nullptr || key[0] == '\0') {
                return false;
            }
            if (SharedPtr<Texture2D> tex = world.TryGetTextureByKeyOrPath(key)) {
                (mat->*setter)(MoveTemp(tex));
                return true;
            }
            if (SharedPtr<Texture2D> loaded = world.LoadTexture(key)) {
                (mat->*setter)(MoveTemp(loaded));
                return true;
            }
            return false;
        };
        if (ctx.assetsRoot != nullptr) {
            const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, texPath);
            if (ctx.assetLoader != nullptr) {
                ctx.assetLoader->RequestTexture(full.CStr());
                if (!tryBind(full.CStr())) {
                    if (ctx.onDeferredComponent != nullptr) {
                        ctx.onDeferredComponent(&owner, ComponentRecord{}, ctx.deferredUserData);
                    }
                }
            } else {
                (void)tryBind(full.CStr());
            }
        } else {
            (void)tryBind(texPath);
        }
    }
};

class PointLightSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PointLight; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "point_light"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const PointLightComponent* pl = owner.GetComponent<PointLightComponent>();
        if (pl == nullptr) {
            return false;
        }
        const Vector3& c = pl->GetColor();
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%.6f %.6f %.6f %.6f %.6f %d %d",
                c.x,
                c.y,
                c.z,
                pl->GetIntensity(),
                pl->GetRange(),
                pl->CastsShadow() ? 1 : 0,
                pl->IsEnabled() ? 1 : 0);
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
        Vector3 color{1.0F, 1.0F, 1.0F};
        float intensity = 2.4F;
        float range = 12.0F;
        int shadow = 0;
        int enabled = 1;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%f %f %f %f %f %d %d",
                    &color.x,
                    &color.y,
                    &color.z,
                    &intensity,
                    &range,
                    &shadow,
                    &enabled)
            < 5) {
            return false;
        }
        PointLightComponent* pl = owner.GetComponent<PointLightComponent>();
        if (pl == nullptr) {
            pl = owner.AddComponent<PointLightComponent>(color, intensity, range);
        } else {
            pl->SetColor(color);
            pl->SetIntensity(intensity);
            pl->SetRange(range);
        }
        pl->SetCastsShadow(shadow != 0);
        pl->SetEnabled(enabled != 0);
        return true;
    }
};

class SkinnedMeshSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::SkinnedMesh; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "skinned_mesh"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const SkinnedMeshComponent* sm = owner.GetComponent<SkinnedMeshComponent>();
        if (sm == nullptr) {
            return false;
        }
        Utf8String asset;
        if (ctx.resolveSkinnedAssetPath != nullptr) {
            asset = ctx.resolveSkinnedAssetPath(owner, ctx.skinnedAssetUserData);
        }
        char buf[400]{};
        std::snprintf(buf, sizeof(buf), "\"%s\"", asset.CStr());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        (void)owner;
        (void)record;
        /** Restored together with animator handler (loads glTF once). */
        return KindTagEquals(record.kind, GetKindTag());
    }
};

class AnimatorSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Animator; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "animator"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const override {
        const AnimatorComponent* anim = owner.GetComponent<AnimatorComponent>();
        if (anim == nullptr) {
            return false;
        }
        Utf8String asset;
        if (ctx.resolveSkinnedAssetPath != nullptr) {
            asset = ctx.resolveSkinnedAssetPath(owner, ctx.skinnedAssetUserData);
        }
        const Utf8String& clipName = anim->GetClipName(anim->GetClipIndex());
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "\"%s\" %u %.6f %u %.6f \"%s\"",
                asset.CStr(),
                anim->GetClipIndex(),
                anim->GetSpeed(),
                static_cast<unsigned>(anim->GetLoopMode()),
                anim->GetTimeSeconds(),
                clipName.CStr());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) const override {
        if (!KindTagEquals(record.kind, GetKindTag()) || ctx.assetsRoot == nullptr) {
            return false;
        }
        char asset[384]{};
        char clipName[128]{};
        unsigned clipIndex = 0;
        unsigned loopMode = 0;
        float speed = 1.0F;
        float timeSec = 0.0F;
        if (std::sscanf(
                    record.payload.CStr(),
                    "\"%383[^\"]\" %u %f %u %f \"%127[^\"]\"",
                    asset,
                    &clipIndex,
                    &speed,
                    &loopMode,
                    &timeSec,
                    clipName)
            < 5) {
            return false;
        }
        if (asset[0] == '\0') {
            return false;
        }
        SkinnedGltfAsset skinned{};
        const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, asset);
        if (ctx.assetLoader != nullptr) {
            ctx.assetLoader->RequestSkinnedGltf(full.CStr());
            if (!world.TryGetCachedSkinnedGltf(full.CStr(), skinned)) {
                if (ctx.onDeferredComponent != nullptr) {
                    ctx.onDeferredComponent(&owner, record, ctx.deferredUserData);
                    return true;
                }
                return false;
            }
        } else {
            skinned = world.LoadSkinnedGltf(full.CStr());
        }
        if (!skinned.mesh || !skinned.skeleton) {
            return false;
        }
        if (owner.GetComponent<SkinnedMeshComponent>() == nullptr) {
            owner.AddComponent<SkinnedMeshComponent>(skinned.mesh);
        }
        GltfAssetBinder::ApplyMaterials(owner, skinned);
        AnimatorComponent* anim = owner.GetComponent<AnimatorComponent>();
        if (anim == nullptr) {
            anim = owner.AddComponent<AnimatorComponent>(skinned.skeleton, clipIndex, speed);
        }
        std::uint32_t resolvedClip = clipIndex;
        if (clipName[0] != '\0') {
            if (const std::int32_t byName = skinned.skeleton->FindClipIndexByName(clipName); byName >= 0) {
                resolvedClip = static_cast<std::uint32_t>(byName);
            }
        }
        anim->SetClipIndex(resolvedClip);
        anim->SetSpeed(speed);
        anim->SetLoopMode(static_cast<AnimLoopMode>(loopMode));
        anim->SetTimeSeconds(timeSec);
        return true;
    }
};

class CameraSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::Camera; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "camera"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const CameraComponent* cam = owner.GetComponent<CameraComponent>();
        if (cam == nullptr) {
            return false;
        }
        char buf[192]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %.6f %.6f %.6f %.6f %d %d",
                static_cast<unsigned>(cam->GetProjectionMode()),
                cam->GetFovYDegrees(),
                cam->GetNearPlane(),
                cam->GetFarPlane(),
                cam->GetOrthoHalfHeight(),
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
        unsigned mode = 0U;
        float fov = 60.0F;
        float nearZ = 0.1F;
        float farZ = 500.0F;
        float orthoHalf = 5.0F;
        int priority = 0;
        int enabled = 1;
        if (std::sscanf(
                    record.payload.CStr(),
                    "%u %f %f %f %f %d %d",
                    &mode,
                    &fov,
                    &nearZ,
                    &farZ,
                    &orthoHalf,
                    &priority,
                    &enabled) < 7) {
            return false;
        }
        if (owner.GetComponent<CameraComponent>() != nullptr) {
            return true;
        }
        CameraComponent* cam = owner.AddComponent<CameraComponent>();
        cam->SetProjectionMode(static_cast<CameraProjectionMode>(mode));
        cam->SetFovYDegrees(fov);
        cam->SetNearPlane(nearZ);
        cam->SetFarPlane(farZ);
        cam->SetOrthoHalfHeight(orthoHalf);
        cam->SetPriority(static_cast<std::int32_t>(priority));
        cam->SetEnabled(enabled != 0);
        return true;
    }
};

void RegisterBuiltInHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<TransformSnapshotHandler>(registry);
    RegisterHandler<MeshSnapshotHandler>(registry);
    RegisterHandler<MaterialSnapshotHandler>(registry);
    RegisterHandler<PointLightSnapshotHandler>(registry);
    RegisterHandler<CameraSnapshotHandler>(registry);
    RegisterHandler<SkinnedMeshSnapshotHandler>(registry);
    RegisterHandler<AnimatorSnapshotHandler>(registry);
    RegisterExtendedSnapshotHandlers(registry);
    RegisterMoreSnapshotHandlers(registry);
    RegisterRenderingSnapshotHandlers(registry);
}

}  // namespace

ComponentSnapshotRegistry::ComponentSnapshotRegistry() {
    RegisterBuiltInHandlers(*this);
}

ComponentSnapshotRegistry::~ComponentSnapshotRegistry() = default;

void ComponentSnapshotRegistry::Register(UniquePtr<IComponentSnapshotHandler> handler) {
    if (handler) {
        handlers.PushBack(MoveTemp(handler));
    }
}

const IComponentSnapshotHandler* ComponentSnapshotRegistry::Find(const ComponentKind kind) const noexcept {
    for (std::size_t i = 0; i < handlers.GetSize(); ++i) {
        if (handlers[i] && handlers[i]->GetKind() == kind) {
            return handlers[i].Get();
        }
    }
    return nullptr;
}

const IComponentSnapshotHandler* ComponentSnapshotRegistry::FindByTag(const char* tag) const noexcept {
    if (tag == nullptr) {
        return nullptr;
    }
    for (std::size_t i = 0; i < handlers.GetSize(); ++i) {
        if (handlers[i] && std::strcmp(handlers[i]->GetKindTag(), tag) == 0) {
            return handlers[i].Get();
        }
    }
    return nullptr;
}

const IComponentSnapshotHandler* ComponentSnapshotRegistry::GetHandlerAt(const std::size_t index) const noexcept {
    if (index >= handlers.GetSize()) {
        return nullptr;
    }
    return handlers[index].Get();
}

ComponentSnapshotRegistry& ComponentSnapshotRegistry::Default() {
    static ComponentSnapshotRegistry instance;
    return instance;
}

}  // namespace Spark
