#include "spark/scene/material/GltfMaterial.hpp"

#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"

#include "cgltf.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

struct GltfTextureLoadCaches {
    HashMap<const cgltf_image*, SharedPtr<Texture2D>> decodedImages;
    HashMap<std::uint64_t, SharedPtr<Texture2D>> mergedOrmTextures;
};

[[nodiscard]] std::uint64_t MakeOrmCacheKey(const cgltf_image* mrImage, const cgltf_image* occImage) noexcept {
    const auto pack = [](const void* ptr) -> std::uint64_t {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
    };
    return (pack(mrImage) << 1U) ^ pack(occImage);
}

}  // namespace

namespace {

Utf8String ParentDirectory(const char* filePath) {
    if (filePath == nullptr) {
        return {};
    }
    const char* last = nullptr;
    for (const char* p = filePath; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            last = p;
        }
    }
    if (last == nullptr) {
        return {};
    }
    Utf8String out;
    for (const char* q = filePath; q < last; ++q) {
        const char unit[2] = {*q, '\0'};
        out.AppendUtf8(unit);
    }
    return out;
}

Utf8String MakeTextureName(const char* gltfPath, const cgltf_image* image, const char* slotLabel) {
    Utf8String name;
    if (gltfPath != nullptr && gltfPath[0] != '\0') {
        name.AppendUtf8(gltfPath);
        name.AppendUtf8("#");
    }
    if (slotLabel != nullptr) {
        name.AppendUtf8(slotLabel);
        name.AppendUtf8(":");
    }
    if (image != nullptr && image->uri != nullptr && image->uri[0] != '\0') {
        name.AppendUtf8(image->uri);
    } else if (image != nullptr && image->buffer_view != nullptr) {
        name.AppendUtf8("embedded");
    } else {
        name.AppendUtf8("unknown");
    }
    return name;
}

[[nodiscard]] int Base64Value(char c) noexcept {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

bool TryDecodeBase64(const char* encoded, Array<std::uint8_t>& outBytes) {
    outBytes.Clear();
    if (encoded == nullptr) {
        return false;
    }
    int val = 0;
    int valb = -8;
    for (const char* p = encoded; *p != '\0'; ++p) {
        if (*p == '=') {
            break;
        }
        const int cv = Base64Value(*p);
        if (cv < 0) {
            continue;
        }
        val = (val << 6) + cv;
        valb += 6;
        if (valb >= 0) {
            outBytes.PushBack(static_cast<std::uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return !outBytes.IsEmpty();
}

bool TryDecodeDataUriImage(const char* uri, Texture2D& outDecoded) {
    if (uri == nullptr || std::strncmp(uri, "data:", 5) != 0) {
        return false;
    }
    const char* comma = std::strchr(uri, ',');
    if (comma == nullptr) {
        return false;
    }
    if (std::strstr(uri, ";base64") == nullptr) {
        return false;
    }
    Array<std::uint8_t> bytes;
    if (!TryDecodeBase64(comma + 1, bytes)) {
        return false;
    }
    return Texture2D::TryLoadFromMemory(bytes.GetData(), bytes.GetSize(), outDecoded, "data-uri");
}

SharedPtr<Texture2D> CreateMergedOrmTexture(
        const SharedPtr<Texture2D>& metallicRoughness,
        const SharedPtr<Texture2D>& occlusion,
        const float roughnessFactor,
        const float metallicFactor,
        const Utf8String& textureName) {
    if (!occlusion || occlusion->GetWidth() == 0 || occlusion->GetHeight() == 0) {
        return metallicRoughness;
    }
    const std::uint32_t w = occlusion->GetWidth();
    const std::uint32_t h = occlusion->GetHeight();
    const Array<std::uint8_t>& occ = occlusion->GetRgba();
    Array<std::uint8_t> pixels;
    pixels.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    if (metallicRoughness && metallicRoughness->GetWidth() == w && metallicRoughness->GetHeight() == h &&
        metallicRoughness->GetRgba().GetSize() == pixels.GetSize()) {
        pixels = metallicRoughness->GetRgba();
    } else {
        for (std::uint32_t y = 0; y < h; ++y) {
            for (std::uint32_t x = 0; x < w; ++x) {
                const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
                const std::uint8_t ao = occ[i];
                pixels[i + 0] = ao;
                pixels[i + 1] = static_cast<std::uint8_t>(
                        std::clamp(roughnessFactor, 0.0F, 1.0F) * 255.0F);
                pixels[i + 2] = static_cast<std::uint8_t>(std::clamp(metallicFactor, 0.0F, 1.0F) * 255.0F);
                pixels[i + 3] = 255;
            }
        }
    }
    for (std::size_t i = 0; i < pixels.GetSize(); i += 4U) {
        pixels[i] = occ[i];
    }
    auto tex = MakeShared<Texture2D>(textureName);
    tex->SetPixels(w, h, MoveTemp(pixels));
    return tex;
}

SharedPtr<Texture2D> GetOrCreateMergedOrmTexture(
        GltfTextureLoadCaches& caches,
        const SharedPtr<Texture2D>& metallicRoughness,
        const SharedPtr<Texture2D>& occlusion,
        const cgltf_image* mrImage,
        const cgltf_image* occImage,
        const float roughnessFactor,
        const float metallicFactor,
        const Utf8String& textureName) {
    const std::uint64_t key = MakeOrmCacheKey(mrImage, occImage);
    if (const SharedPtr<Texture2D>* cached = caches.mergedOrmTextures.Find(key)) {
        return *cached;
    }
    SharedPtr<Texture2D> merged =
            CreateMergedOrmTexture(metallicRoughness, occlusion, roughnessFactor, metallicFactor, textureName);
    caches.mergedOrmTextures.Add(key, merged);
    return merged;
}

bool TryDecodeGltfImage(const cgltf_image* img, const Utf8String& dir, Texture2D& outDecoded) {
    if (img == nullptr) {
        return false;
    }
    if (img->buffer_view != nullptr) {
        const cgltf_buffer_view* bv = img->buffer_view;
        if (bv->buffer == nullptr || bv->buffer->data == nullptr) {
            return false;
        }
        const auto* bytes =
                static_cast<const std::uint8_t*>(bv->buffer->data) + static_cast<std::size_t>(bv->offset);
        const std::size_t sz = static_cast<std::size_t>(bv->size);
        const char* debugName = outDecoded.GetName().IsEmpty() ? "glTF" : outDecoded.GetName().CStr();
        return Texture2D::TryLoadFromMemory(bytes, sz, outDecoded, debugName);
    }
    if (img->uri != nullptr) {
        if (std::strncmp(img->uri, "data:", 5) == 0) {
            return TryDecodeDataUriImage(img->uri, outDecoded);
        }
        Utf8String full;
        if (dir.IsEmpty()) {
            full = Utf8String(img->uri);
        } else {
            full.AppendUtf8(dir.CStr());
            full.AppendUtf8("/");
            full.AppendUtf8(img->uri);
        }
        return Texture2D::TryLoadFromFile(full.CStr(), outDecoded, false);
    }
    return false;
}

bool TryGetOrDecodeTextureView(
        const cgltf_texture_view& tv,
        const Utf8String& dir,
        const Utf8String& textureName,
        GltfTextureLoadCaches& caches,
        SharedPtr<Texture2D>& outTexture) {
    if (tv.texture == nullptr || tv.texture->image == nullptr) {
        return false;
    }
    const cgltf_image* image = tv.texture->image;
    if (const SharedPtr<Texture2D>* cached = caches.decodedImages.Find(image)) {
        outTexture = *cached;
        return true;
    }
    Texture2D decoded(textureName);
    if (!TryDecodeGltfImage(image, dir, decoded)) {
        return false;
    }
    SharedPtr<Texture2D> tex(new Texture2D(MoveTemp(decoded)));
    caches.decodedImages.Add(image, tex);
    outTexture = tex;
    return true;
}

void ReadTextureViewUv(const cgltf_texture_view& tv, MaterialUvMap& out) noexcept {
    out.texCoordSet = static_cast<std::uint32_t>(tv.texcoord);
    if (tv.has_transform) {
        const cgltf_texture_transform& xform = tv.transform;
        out.uvOffset = {static_cast<float>(xform.offset[0]), static_cast<float>(xform.offset[1])};
        out.uvScale = {static_cast<float>(xform.scale[0]), static_cast<float>(xform.scale[1])};
        out.uvRotation = static_cast<float>(xform.rotation);
        if (xform.has_texcoord) {
            out.texCoordSet = static_cast<std::uint32_t>(xform.texcoord);
        }
    }
}

bool ApproximatelyEqualUv(const float a, const float b) noexcept {
    return std::fabs(a - b) <= 1.0e-5F;
}

bool UvMapsMatch(const MaterialUvMap& a, const MaterialUvMap& b) noexcept {
    return a.texCoordSet == b.texCoordSet &&
           ApproximatelyEqualUv(a.uvScale.x, b.uvScale.x) && ApproximatelyEqualUv(a.uvScale.y, b.uvScale.y) &&
           ApproximatelyEqualUv(a.uvOffset.x, b.uvOffset.x) && ApproximatelyEqualUv(a.uvOffset.y, b.uvOffset.y) &&
           ApproximatelyEqualUv(a.uvRotation, b.uvRotation);
}

void ApplyScalarFactors(const cgltf_material& mat, GltfMaterial& out) {
    if (mat.has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = mat.pbr_metallic_roughness;
        out.baseColorFactor = {
                static_cast<float>(pbr.base_color_factor[0]),
                static_cast<float>(pbr.base_color_factor[1]),
                static_cast<float>(pbr.base_color_factor[2])};
        out.opacity = static_cast<float>(pbr.base_color_factor[3]);
        out.metallicFactor = static_cast<float>(pbr.metallic_factor);
        out.roughnessFactor = static_cast<float>(pbr.roughness_factor);
    }
    out.emissiveFactor = {
            static_cast<float>(mat.emissive_factor[0]),
            static_cast<float>(mat.emissive_factor[1]),
            static_cast<float>(mat.emissive_factor[2])};
    const float emissiveLen = std::sqrt(
            out.emissiveFactor.x * out.emissiveFactor.x + out.emissiveFactor.y * out.emissiveFactor.y +
            out.emissiveFactor.z * out.emissiveFactor.z);
    if (emissiveLen > 1.0e-6F) {
        out.emissiveIntensity = emissiveLen;
        out.emissiveFactor.x /= emissiveLen;
        out.emissiveFactor.y /= emissiveLen;
        out.emissiveFactor.z /= emissiveLen;
    } else {
        out.emissiveIntensity = 0.0F;
    }
    out.doubleSided = mat.double_sided;
    out.unlit = mat.unlit != 0;
    if (mat.occlusion_texture.texture != nullptr) {
        out.occlusionStrength = static_cast<float>(mat.occlusion_texture.scale);
    }
    if (mat.alpha_mode == cgltf_alpha_mode_mask) {
        out.alphaCutoff = static_cast<float>(mat.alpha_cutoff);
    } else if (mat.alpha_mode == cgltf_alpha_mode_blend) {
        out.alphaBlend = true;
    }
}

void ApplyGltfExtensions(const cgltf_material& mat, GltfMaterial& out) {
    out.gltfExtensions = MaterialGltfExtensions{};
    if (mat.has_clearcoat) {
        out.gltfExtensions.clearcoatFactor = static_cast<float>(mat.clearcoat.clearcoat_factor);
        out.gltfExtensions.clearcoatRoughnessFactor = static_cast<float>(mat.clearcoat.clearcoat_roughness_factor);
    }
    if (mat.has_transmission) {
        out.gltfExtensions.transmissionFactor = static_cast<float>(mat.transmission.transmission_factor);
    }
    if (mat.has_emissive_strength) {
        out.gltfExtensions.emissiveStrength = static_cast<float>(mat.emissive_strength.emissive_strength);
    }
    if (mat.has_iridescence) {
        const cgltf_iridescence& ir = mat.iridescence;
        out.gltfExtensions.iridescenceFactor = static_cast<float>(ir.iridescence_factor);
        out.gltfExtensions.iridescenceIor = static_cast<float>(ir.iridescence_ior);
        out.gltfExtensions.iridescenceThicknessMin = static_cast<float>(ir.iridescence_thickness_min);
        out.gltfExtensions.iridescenceThicknessMax = static_cast<float>(ir.iridescence_thickness_max);
    }
}

bool TryLoadTexturesFromMaterial(
        const cgltf_material& mat,
        const char* gltfPath,
        GltfMaterial& out,
        GltfTextureLoadCaches& caches) {
    const Utf8String dir = ParentDirectory(gltfPath);
    ApplyScalarFactors(mat, out);
    ApplyGltfExtensions(mat, out);

    const cgltf_image* mrImage = nullptr;
    const cgltf_image* occImage = nullptr;
    SharedPtr<Texture2D> occlusion;

    if (mat.has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = mat.pbr_metallic_roughness;
        if (pbr.base_color_texture.texture != nullptr) {
            const Utf8String name = MakeTextureName(gltfPath, pbr.base_color_texture.texture->image, "base");
            ReadTextureViewUv(pbr.base_color_texture, out.baseColorUv);
            (void)TryGetOrDecodeTextureView(pbr.base_color_texture, dir, name, caches, out.baseColor);
        }
        if (pbr.metallic_roughness_texture.texture != nullptr) {
            mrImage = pbr.metallic_roughness_texture.texture->image;
            const Utf8String name = MakeTextureName(gltfPath, mrImage, "orm");
            ReadTextureViewUv(pbr.metallic_roughness_texture, out.metallicRoughnessUv);
            (void)TryGetOrDecodeTextureView(pbr.metallic_roughness_texture, dir, name, caches, out.metallicRoughness);
        }
    }

    if (mat.normal_texture.texture != nullptr) {
        const Utf8String name = MakeTextureName(gltfPath, mat.normal_texture.texture->image, "normal");
        out.normalScale = static_cast<float>(mat.normal_texture.scale);
        ReadTextureViewUv(mat.normal_texture, out.normalUv);
        (void)TryGetOrDecodeTextureView(mat.normal_texture, dir, name, caches, out.normalMap);
    }

    if (mat.emissive_texture.texture != nullptr) {
        const Utf8String name = MakeTextureName(gltfPath, mat.emissive_texture.texture->image, "emissive");
        ReadTextureViewUv(mat.emissive_texture, out.emissiveUv);
        (void)TryGetOrDecodeTextureView(mat.emissive_texture, dir, name, caches, out.emissiveMap);
    }

    if (mat.has_iridescence && mat.iridescence.iridescence_thickness_texture.texture != nullptr) {
        const Utf8String name =
                MakeTextureName(gltfPath, mat.iridescence.iridescence_thickness_texture.texture->image, "iridescence");
        ReadTextureViewUv(mat.iridescence.iridescence_thickness_texture, out.iridescenceThicknessUv);
        (void)TryGetOrDecodeTextureView(
                mat.iridescence.iridescence_thickness_texture, dir, name, caches, out.iridescenceThicknessMap);
    }

    const bool hasMetallicRoughnessTexture =
            mat.has_pbr_metallic_roughness &&
            mat.pbr_metallic_roughness.metallic_roughness_texture.texture != nullptr;
    if (mat.occlusion_texture.texture != nullptr && out.occlusionStrength > 1.0e-6F) {
        occImage = mat.occlusion_texture.texture->image;
        const Utf8String name = MakeTextureName(gltfPath, occImage, "occlusion");
        MaterialUvMap occlusionUv{};
        ReadTextureViewUv(mat.occlusion_texture, occlusionUv);
        if (TryGetOrDecodeTextureView(mat.occlusion_texture, dir, name, caches, occlusion)) {
            if (hasMetallicRoughnessTexture) {
                if (UvMapsMatch(occlusionUv, out.metallicRoughnessUv)) {
                    out.metallicRoughness = GetOrCreateMergedOrmTexture(
                            caches,
                            out.metallicRoughness,
                            occlusion,
                            mrImage,
                            occImage,
                            out.roughnessFactor,
                            out.metallicFactor,
                            name);
                }
            } else {
                out.metallicRoughness = GetOrCreateMergedOrmTexture(
                        caches,
                        out.metallicRoughness,
                        occlusion,
                        mrImage,
                        occImage,
                        out.roughnessFactor,
                        out.metallicFactor,
                        name);
                out.metallicRoughnessUv = occlusionUv;
            }
        }
    }

    if (!mat.has_pbr_metallic_roughness && mat.has_pbr_specular_glossiness) {
        const cgltf_pbr_specular_glossiness& sg = mat.pbr_specular_glossiness;
        out.baseColorFactor = {
                static_cast<float>(sg.diffuse_factor[0]),
                static_cast<float>(sg.diffuse_factor[1]),
                static_cast<float>(sg.diffuse_factor[2])};
        out.opacity = static_cast<float>(sg.diffuse_factor[3]);
        out.metallicFactor = 0.0F;
        out.roughnessFactor = 1.0F - static_cast<float>(sg.glossiness_factor);
        if (sg.diffuse_texture.texture != nullptr) {
            const Utf8String name = MakeTextureName(gltfPath, sg.diffuse_texture.texture->image, "diffuse");
            ReadTextureViewUv(sg.diffuse_texture, out.baseColorUv);
            (void)TryGetOrDecodeTextureView(sg.diffuse_texture, dir, name, caches, out.baseColor);
        }
    }

    if (!mat.has_pbr_metallic_roughness && mat.has_sheen) {
        if (mat.sheen.sheen_color_texture.texture != nullptr && !out.baseColor) {
            const Utf8String name = MakeTextureName(gltfPath, mat.sheen.sheen_color_texture.texture->image, "sheen");
            (void)TryGetOrDecodeTextureView(mat.sheen.sheen_color_texture, dir, name, caches, out.baseColor);
        }
    }

    return true;
}

}  // namespace

namespace {

bool ApproximatelyEqual(const float a, const float b) noexcept {
    return std::fabs(a - b) <= 1.0e-5F;
}

}  // namespace

bool GltfMaterial::HasScalarPresentation() const noexcept {
    if (!ApproximatelyEqual(baseColorFactor.x, 1.0F) || !ApproximatelyEqual(baseColorFactor.y, 1.0F) ||
        !ApproximatelyEqual(baseColorFactor.z, 1.0F) || opacity < 0.999F) {
        return true;
    }
    if (!ApproximatelyEqual(metallicFactor, 1.0F) || !ApproximatelyEqual(roughnessFactor, 1.0F)) {
        return true;
    }
    if (emissiveIntensity > 1.0e-6F) {
        return true;
    }
    if (!ApproximatelyEqual(occlusionStrength, 1.0F)) {
        return true;
    }
    if (doubleSided || alphaCutoff > 1.0e-6F || alphaBlend) {
        return true;
    }
    if (gltfExtensions.HasClearcoat() || gltfExtensions.HasTransmission() || gltfExtensions.HasEmissiveStrength() ||
        gltfExtensions.HasIridescence()) {
        return true;
    }
    return false;
}

void GltfMaterial::ApplyTo(MaterialComponent& material) const {
    if (baseColor) {
        material.SetBaseColorTexture(baseColor);
    }
    if (normalMap) {
        material.SetNormalTexture(normalMap);
    }
    if (metallicRoughness) {
        material.SetMetallicRoughnessTexture(metallicRoughness);
        material.SetMetallic(1.0F);
        material.SetRoughness(1.0F);
        material.SetMetallicFactor(metallicFactor);
        material.SetRoughnessFactor(roughnessFactor);
    } else {
        material.SetMetallic(metallicFactor);
        material.SetRoughness(roughnessFactor);
        material.SetMetallicFactor(1.0F);
        material.SetRoughnessFactor(1.0F);
    }
    if (emissiveMap) {
        material.SetEmissiveTexture(emissiveMap);
    }
    if (iridescenceThicknessMap) {
        material.SetIridescenceThicknessTexture(iridescenceThicknessMap);
    }
    material.SetTint(baseColorFactor);
    material.SetOcclusionStrength(occlusionStrength);
    material.SetEmissive(emissiveFactor, emissiveIntensity);
    material.SetEmissiveFactor(emissiveFactor);
    material.SetDoubleSided(doubleSided);
    material.SetOpacity(opacity);
    material.SetAlphaCutoff(alphaCutoff);
    material.SetAlphaBlend(alphaBlend);
    material.SetBaseColorUvMap(baseColorUv);
    material.SetNormalUvMap(normalUv);
    material.SetMetallicRoughnessUvMap(metallicRoughnessUv);
    material.SetEmissiveUvMap(emissiveUv);
    material.SetIridescenceThicknessUvMap(iridescenceThicknessUv);
    material.SetNormalScale(normalScale);
    material.SetGltfExtensions(gltfExtensions);
    if (unlit) {
        material.SetShadingModel(SceneShadingModel::Unlit);
    }
}

void GltfMaterial::ApplyTo(MultiMaterialComponent::Slot& slot) const {
    slot.baseColor = baseColor;
    slot.normalMap = normalMap;
    slot.metallicRoughness = metallicRoughness;
    slot.emissiveMap = emissiveMap;
    slot.iridescenceThicknessMap = iridescenceThicknessMap;
    slot.tint = baseColorFactor;
    if (metallicRoughness) {
        slot.metallic = 1.0F;
        slot.roughness = 1.0F;
        slot.metallicFactor = metallicFactor;
        slot.roughnessFactor = roughnessFactor;
    } else {
        slot.metallic = metallicFactor;
        slot.roughness = roughnessFactor;
        slot.metallicFactor = 1.0F;
        slot.roughnessFactor = 1.0F;
    }
    slot.occlusionStrength = occlusionStrength;
    slot.emissiveColor = emissiveFactor;
    slot.emissiveIntensity = emissiveIntensity;
    slot.emissiveFactor = emissiveFactor;
    slot.doubleSided = doubleSided;
    slot.opacity = opacity;
    slot.alphaCutoff = alphaCutoff;
    slot.alphaBlend = alphaBlend;
    slot.baseColorUv = baseColorUv;
    slot.normalUv = normalUv;
    slot.metallicRoughnessUv = metallicRoughnessUv;
    slot.emissiveUv = emissiveUv;
    slot.iridescenceThicknessUv = iridescenceThicknessUv;
    slot.normalScale = normalScale;
    slot.gltfExtensions = gltfExtensions;
    if (unlit) {
        slot.shadingModel = SceneShadingModel::Unlit;
    }
}

bool GltfMaterialLoader::LoadFromCgltf(
        const cgltf_material* mat,
        const char* gltfPath,
        GltfMaterial& outMaterial) {
    if (mat == nullptr) {
        return false;
    }
    outMaterial = GltfMaterial{};
    GltfTextureLoadCaches caches;
    return TryLoadTexturesFromMaterial(*mat, gltfPath, outMaterial, caches);
}

bool GltfMaterialLoader::LoadPrimary(
        const cgltf_data* data,
        const cgltf_mesh* meshHint,
        const char* gltfPath,
        GltfMaterial& outMaterial) {
    if (data == nullptr || gltfPath == nullptr) {
        return false;
    }
    outMaterial = GltfMaterial{};

    if (meshHint != nullptr) {
        for (cgltf_size pi = 0; pi < meshHint->primitives_count; ++pi) {
            const cgltf_primitive& prim = meshHint->primitives[pi];
            if (prim.material != nullptr) {
                return LoadFromCgltf(prim.material, gltfPath, outMaterial);
            }
        }
    }

    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        if (LoadFromCgltf(&data->materials[mi], gltfPath, outMaterial)) {
            if (outMaterial.HasAnyTexture() || data->materials[mi].has_pbr_metallic_roughness) {
                return true;
            }
        }
    }

    return outMaterial.HasAnyTexture();
}

void GltfMaterialLoader::LoadAll(
        const cgltf_data* data,
        const char* gltfPath,
        Array<GltfMaterial>& outMaterials) {
    outMaterials.Clear();
    if (data == nullptr || gltfPath == nullptr) {
        return;
    }
    GltfTextureLoadCaches caches;
    outMaterials.Resize(static_cast<std::size_t>(data->materials_count));
    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        (void)TryLoadTexturesFromMaterial(
                data->materials[mi], gltfPath, outMaterials[static_cast<std::size_t>(mi)], caches);
    }
}

void GltfMaterialLoader::LoadVariantNames(const cgltf_data* data, Array<Utf8String>& outVariantNames) {
    outVariantNames.Clear();
    if (data == nullptr) {
        return;
    }
    outVariantNames.Resize(static_cast<std::size_t>(data->variants_count));
    for (cgltf_size vi = 0; vi < data->variants_count; ++vi) {
        const char* name = data->variants[vi].name;
        outVariantNames[static_cast<std::size_t>(vi)] =
                (name != nullptr && name[0] != '\0') ? Utf8String(name) : Utf8String{};
    }
}

}  // namespace Spark
