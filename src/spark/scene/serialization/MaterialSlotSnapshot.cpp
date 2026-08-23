#include "spark/scene/MaterialAsset.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/GameWorldAssetLoader.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Spark {

namespace {

bool IsRegularFile(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    struct stat st {};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
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

Utf8String ResolveTexturePath(
        const SharedPtr<Texture2D>& texture,
        const GameObject& owner,
        const SceneCaptureContext& ctx) {
    if (texture) {
        return texture->GetName();
    }
    if (ctx.resolveTexturePath != nullptr) {
        return ctx.resolveTexturePath(owner, ctx.textureUserData);
    }
    return {};
}

Utf8String MakeRelativeTexturePath(const Utf8String& absolutePath, const char* relativeToDir) {
    if (absolutePath.IsEmpty()) {
        return {};
    }
    if (relativeToDir == nullptr || relativeToDir[0] == '\0') {
        return absolutePath;
    }
    Utf8String base(relativeToDir);
    if (!base.IsEmpty()) {
        const std::size_t n = base.ByteLength();
        const char last = base.CStr()[n - 1];
        if (last != '/' && last != '\\') {
            base.AppendUtf8("/");
        }
    }
    const char* abs = absolutePath.CStr();
    const char* baseC = base.CStr();
    const std::size_t baseLen = base.ByteLength();
    if (baseLen > 0 && std::strncmp(abs, baseC, baseLen) == 0) {
        Utf8String relative;
        relative.AppendUtf8(abs + baseLen);
        return relative;
    }
    return absolutePath;
}

Utf8String ResolveTexturePathForAsset(
        const SharedPtr<Texture2D>& texture,
        const char* relativeToDir,
        const GameWorldAssetCache* textureKeys) {
    if (!texture) {
        return {};
    }
    if (textureKeys != nullptr) {
        const Utf8String cacheKey = textureKeys->TryFindTextureKey(texture.Get());
        return MakeRelativeTexturePath(cacheKey, relativeToDir);
    }
    return MakeRelativeTexturePath(texture->GetName(), relativeToDir);
}

void BindTexturePath(
        GameObject& owner,
        GameWorld& world,
        const SceneApplyContext& ctx,
        const char* texPath,
        SharedPtr<Texture2D>& outTexture) {
    if (texPath == nullptr || texPath[0] == '\0') {
        return;
    }
    auto tryBind = [&](const char* key) -> bool {
        if (key == nullptr || key[0] == '\0') {
            return false;
        }
        if (SharedPtr<Texture2D> tex = world.TryGetTextureByKeyOrPath(key)) {
            outTexture = MoveTemp(tex);
            return true;
        }
        if (SharedPtr<Texture2D> loaded = world.LoadTexture(key)) {
            outTexture = MoveTemp(loaded);
            return true;
        }
        return false;
    };
    if (ctx.assetsRoot != nullptr) {
        const Utf8String full = JoinAssetsRootPath(ctx.assetsRoot, texPath);
        if (IsRegularFile(full.CStr())) {
            if (ctx.assetLoader != nullptr) {
                ctx.assetLoader->RequestTexture(full.CStr());
            }
            if (!tryBind(full.CStr()) && ctx.assetLoader != nullptr && ctx.onDeferredComponent != nullptr) {
                ctx.onDeferredComponent(&owner, ComponentRecord{}, ctx.deferredUserData);
            }
        } else if (!tryBind(texPath) && ctx.assetLoader != nullptr && ctx.onDeferredComponent != nullptr) {
            ctx.onDeferredComponent(&owner, ComponentRecord{}, ctx.deferredUserData);
        }
    } else {
        (void)tryBind(texPath);
    }
}

void ApplyScalarsToMaterial(MaterialComponent& material, const MaterialSlotSnapshot::Data& data) {
    material.SetTint(data.tint);
    material.SetMetallic(data.metallic);
    material.SetRoughness(data.roughness);
    material.SetMetallicFactor(data.metallicFactor);
    material.SetRoughnessFactor(data.roughnessFactor);
    material.SetOcclusionStrength(data.occlusionStrength);
    material.SetEmissive(data.emissiveColor, data.emissiveIntensity);
    material.SetEmissiveFactor(data.emissiveFactor);
    material.SetShadingModel(data.shadingModel);
    material.SetDoubleSided(data.doubleSided);
    material.SetOpacity(data.opacity);
    material.SetAlphaCutoff(data.alphaCutoff);
}

void ApplyScalarsToSlot(MultiMaterialComponent::Slot& slot, const MaterialSlotSnapshot::Data& data) {
    slot.tint = data.tint;
    slot.metallic = data.metallic;
    slot.roughness = data.roughness;
    slot.metallicFactor = data.metallicFactor;
    slot.roughnessFactor = data.roughnessFactor;
    slot.occlusionStrength = data.occlusionStrength;
    slot.emissiveColor = data.emissiveColor;
    slot.emissiveIntensity = data.emissiveIntensity;
    slot.emissiveFactor = data.emissiveFactor;
    slot.shadingModel = data.shadingModel;
    slot.doubleSided = data.doubleSided;
    slot.opacity = data.opacity;
    slot.alphaCutoff = data.alphaCutoff;
}

}  // namespace

void MaterialSlotSnapshot::CaptureFromMaterial(
        const MaterialComponent& material,
        const GameObject& owner,
        const SceneCaptureContext& ctx,
        Data& out) {
    out = Data{};
    out.baseColorPath = ResolveTexturePath(material.GetBaseColorTexture(), owner, ctx);
    out.normalPath = ResolveTexturePath(material.GetNormalTexture(), owner, ctx);
    out.metallicRoughnessPath = ResolveTexturePath(material.GetMetallicRoughnessTexture(), owner, ctx);
    out.emissivePath = ResolveTexturePath(material.GetEmissiveTexture(), owner, ctx);
    out.tint = material.GetTint();
    out.metallic = material.GetMetallic();
    out.roughness = material.GetRoughness();
    out.metallicFactor = material.GetMetallicFactor();
    out.roughnessFactor = material.GetRoughnessFactor();
    out.occlusionStrength = material.GetOcclusionStrength();
    out.emissiveColor = material.GetEmissiveColor();
    out.emissiveIntensity = material.GetEmissiveIntensity();
    out.emissiveFactor = material.GetEmissiveFactor();
    out.doubleSided = material.IsDoubleSided();
    out.opacity = material.GetOpacity();
    out.alphaCutoff = material.GetAlphaCutoff();
    out.shadingModel = material.GetShadingModel();
}

void MaterialSlotSnapshot::CaptureFromSlot(
        const MultiMaterialComponent::Slot& slot,
        const SceneCaptureContext& ctx,
        const GameObject& owner,
        Data& out) {
    out = Data{};
    out.baseColorPath = ResolveTexturePath(slot.baseColor, owner, ctx);
    out.normalPath = ResolveTexturePath(slot.normalMap, owner, ctx);
    out.metallicRoughnessPath = ResolveTexturePath(slot.metallicRoughness, owner, ctx);
    out.emissivePath = ResolveTexturePath(slot.emissiveMap, owner, ctx);
    out.tint = slot.tint;
    out.metallic = slot.metallic;
    out.roughness = slot.roughness;
    out.metallicFactor = slot.metallicFactor;
    out.roughnessFactor = slot.roughnessFactor;
    out.occlusionStrength = slot.occlusionStrength;
    out.emissiveColor = slot.emissiveColor;
    out.emissiveIntensity = slot.emissiveIntensity;
    out.emissiveFactor = slot.emissiveFactor;
    out.doubleSided = slot.doubleSided;
    out.opacity = slot.opacity;
    out.alphaCutoff = slot.alphaCutoff;
    out.shadingModel = slot.shadingModel;
}

void MaterialSlotSnapshot::CaptureFromAsset(
        const MaterialAsset& asset,
        Data& out,
        const char* relativeToDir,
        const GameWorldAssetCache* textureKeys) {
    out = Data{};
    out.baseColorPath = ResolveTexturePathForAsset(asset.baseColor, relativeToDir, textureKeys);
    out.normalPath = ResolveTexturePathForAsset(asset.normalMap, relativeToDir, textureKeys);
    out.metallicRoughnessPath = ResolveTexturePathForAsset(asset.metallicRoughness, relativeToDir, textureKeys);
    out.emissivePath = ResolveTexturePathForAsset(asset.emissiveMap, relativeToDir, textureKeys);
    out.tint = asset.tint;
    out.metallic = asset.metallic;
    out.roughness = asset.roughness;
    out.metallicFactor = asset.metallicFactor;
    out.roughnessFactor = asset.roughnessFactor;
    out.occlusionStrength = asset.occlusionStrength;
    out.emissiveColor = asset.emissiveColor;
    out.emissiveIntensity = asset.emissiveIntensity;
    out.emissiveFactor = asset.emissiveFactor;
    out.doubleSided = asset.doubleSided;
    out.opacity = asset.opacity;
    out.alphaCutoff = asset.alphaCutoff;
    out.shadingModel = asset.shadingModel;
}

void MaterialSlotSnapshot::AppendSlotV1(const Data& data, Utf8String& out) {
    char buf[2048]{};
    const int shadingModel = static_cast<int>(data.shadingModel);
    const int written = std::snprintf(
            buf,
            sizeof(buf),
            "\"%s\" \"%s\" \"%s\" \"%s\" %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f "
            "%.6f %.6f %.6f %.6f %.6f %d %.6f %.6f %d",
            data.baseColorPath.CStr(),
            data.normalPath.CStr(),
            data.metallicRoughnessPath.CStr(),
            data.emissivePath.CStr(),
            data.tint.x,
            data.tint.y,
            data.tint.z,
            data.metallic,
            data.roughness,
            data.metallicFactor,
            data.roughnessFactor,
            data.occlusionStrength,
            data.emissiveColor.x,
            data.emissiveColor.y,
            data.emissiveColor.z,
            data.emissiveIntensity,
            data.emissiveFactor.x,
            data.emissiveFactor.y,
            data.emissiveFactor.z,
            data.doubleSided ? 1 : 0,
            data.opacity,
            data.alphaCutoff,
            shadingModel);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buf)) {
        return;
    }
    out.AppendUtf8(buf);
    out.AppendUtf8(" ");
}

bool MaterialSlotSnapshot::TryParseSlotV1(const char*& cursor, Data& out) {
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
    Vector3 tint{Vector3::One};
    float metallic = 0.0F;
    float roughness = 0.45F;
    float metallicFactor = 1.0F;
    float roughnessFactor = 1.0F;
    float occlusionStrength = 1.0F;
    Vector3 emissiveColor{};
    float emissiveIntensity = 0.0F;
    Vector3 emissiveFactor{Vector3::One};
    int doubleSided = 0;
    float opacity = 1.0F;
    float alphaCutoff = 0.0F;
    int shadingModel = 0;
    int consumed = 0;
    if (std::sscanf(
                cursor,
                "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %d %f %f %d%n",
                &tint.x,
                &tint.y,
                &tint.z,
                &metallic,
                &roughness,
                &metallicFactor,
                &roughnessFactor,
                &occlusionStrength,
                &emissiveColor.x,
                &emissiveColor.y,
                &emissiveColor.z,
                &emissiveIntensity,
                &emissiveFactor.x,
                &emissiveFactor.y,
                &emissiveFactor.z,
                &doubleSided,
                &opacity,
                &alphaCutoff,
                &shadingModel,
                &consumed) < 19) {
        return false;
    }
    cursor += consumed;

    out = Data{};
    out.baseColorPath = Utf8String(basePath);
    out.normalPath = Utf8String(normalPath);
    out.metallicRoughnessPath = Utf8String(mrPath);
    out.emissivePath = Utf8String(emissivePath);
    out.tint = tint;
    out.metallic = metallic;
    out.roughness = roughness;
    out.metallicFactor = metallicFactor;
    out.roughnessFactor = roughnessFactor;
    out.occlusionStrength = occlusionStrength;
    out.emissiveColor = emissiveColor;
    out.emissiveIntensity = emissiveIntensity;
    out.emissiveFactor = emissiveFactor;
    out.doubleSided = doubleSided != 0;
    out.opacity = opacity;
    out.alphaCutoff = alphaCutoff;
    out.shadingModel = static_cast<SceneShadingModel>(shadingModel);
    return true;
}

void MaterialSlotSnapshot::ApplyToMaterial(
        MaterialComponent& material,
        const Data& data,
        GameObject& owner,
        GameWorld& world,
        const SceneApplyContext& ctx) {
    ApplyScalarsToMaterial(material, data);
    SharedPtr<Texture2D> baseColor = material.GetBaseColorTexture();
    SharedPtr<Texture2D> normalMap = material.GetNormalTexture();
    SharedPtr<Texture2D> metallicRoughness = material.GetMetallicRoughnessTexture();
    SharedPtr<Texture2D> emissiveMap = material.GetEmissiveTexture();
    BindTexturePath(owner, world, ctx, data.baseColorPath.CStr(), baseColor);
    BindTexturePath(owner, world, ctx, data.normalPath.CStr(), normalMap);
    BindTexturePath(owner, world, ctx, data.metallicRoughnessPath.CStr(), metallicRoughness);
    BindTexturePath(owner, world, ctx, data.emissivePath.CStr(), emissiveMap);
    material.SetBaseColorTexture(MoveTemp(baseColor));
    material.SetNormalTexture(MoveTemp(normalMap));
    material.SetMetallicRoughnessTexture(MoveTemp(metallicRoughness));
    material.SetEmissiveTexture(MoveTemp(emissiveMap));
}

void MaterialSlotSnapshot::ApplyToSlot(
        MultiMaterialComponent::Slot& slot,
        const Data& data,
        GameObject& owner,
        GameWorld& world,
        const SceneApplyContext& ctx) {
    ApplyScalarsToSlot(slot, data);
    BindTexturePath(owner, world, ctx, data.baseColorPath.CStr(), slot.baseColor);
    BindTexturePath(owner, world, ctx, data.normalPath.CStr(), slot.normalMap);
    BindTexturePath(owner, world, ctx, data.metallicRoughnessPath.CStr(), slot.metallicRoughness);
    BindTexturePath(owner, world, ctx, data.emissivePath.CStr(), slot.emissiveMap);
}

}  // namespace Spark
