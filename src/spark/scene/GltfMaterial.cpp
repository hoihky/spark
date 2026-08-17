#include "spark/scene/GltfMaterial.hpp"

#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"

#include "spark/core/Utf8String.hpp"

#include "cgltf.h"

#include <cmath>
#include <cstring>

namespace Spark {

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
        return Texture2D::TryLoadFromMemory(bytes, sz, outDecoded, "glTF");
    }
    if (img->uri != nullptr) {
        if (std::strncmp(img->uri, "data:", 5) == 0) {
            return false;
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

bool TryDecodeTextureView(
        const cgltf_texture_view& tv,
        const Utf8String& dir,
        const Utf8String& textureName,
        SharedPtr<Texture2D>& outTexture) {
    if (tv.texture == nullptr || tv.texture->image == nullptr) {
        return false;
    }
    Texture2D decoded(textureName);
    if (!TryDecodeGltfImage(tv.texture->image, dir, decoded)) {
        return false;
    }
    outTexture = SharedPtr<Texture2D>(new Texture2D(MoveTemp(decoded)));
    return true;
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
    if (mat.occlusion_texture.texture != nullptr) {
        out.occlusionStrength = static_cast<float>(mat.occlusion_texture.scale);
    }
    if (mat.alpha_mode == cgltf_alpha_mode_mask) {
        out.opacity = static_cast<float>(mat.alpha_cutoff);
    }
}

bool TryLoadTexturesFromMaterial(
        const cgltf_material& mat,
        const char* gltfPath,
        GltfMaterial& out) {
    const Utf8String dir = ParentDirectory(gltfPath);
    ApplyScalarFactors(mat, out);

    if (mat.has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = mat.pbr_metallic_roughness;
        if (pbr.base_color_texture.texture != nullptr) {
            const Utf8String name = MakeTextureName(gltfPath, pbr.base_color_texture.texture->image, "base");
            (void)TryDecodeTextureView(pbr.base_color_texture, dir, name, out.baseColor);
        }
        if (pbr.metallic_roughness_texture.texture != nullptr) {
            const Utf8String name = MakeTextureName(gltfPath, pbr.metallic_roughness_texture.texture->image, "orm");
            (void)TryDecodeTextureView(pbr.metallic_roughness_texture, dir, name, out.metallicRoughness);
        }
    }

    if (mat.normal_texture.texture != nullptr) {
        const Utf8String name = MakeTextureName(gltfPath, mat.normal_texture.texture->image, "normal");
        (void)TryDecodeTextureView(mat.normal_texture, dir, name, out.normalMap);
    }

    if (mat.emissive_texture.texture != nullptr) {
        const Utf8String name = MakeTextureName(gltfPath, mat.emissive_texture.texture->image, "emissive");
        (void)TryDecodeTextureView(mat.emissive_texture, dir, name, out.emissiveMap);
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
            (void)TryDecodeTextureView(sg.diffuse_texture, dir, name, out.baseColor);
        }
    }

    if (!mat.has_pbr_metallic_roughness && mat.has_sheen) {
        if (mat.sheen.sheen_color_texture.texture != nullptr && !out.baseColor) {
            const Utf8String name = MakeTextureName(gltfPath, mat.sheen.sheen_color_texture.texture->image, "sheen");
            (void)TryDecodeTextureView(mat.sheen.sheen_color_texture, dir, name, out.baseColor);
        }
    }

    return true;
}

}  // namespace

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
    material.SetTint(baseColorFactor);
    material.SetOcclusionStrength(occlusionStrength);
    material.SetEmissive(emissiveFactor, emissiveIntensity);
    material.SetEmissiveFactor(emissiveFactor);
    material.SetDoubleSided(doubleSided);
    material.SetOpacity(opacity);
}

void GltfMaterial::ApplyTo(MultiMaterialComponent::Slot& slot) const {
    slot.baseColor = baseColor;
    slot.normalMap = normalMap;
    slot.metallicRoughness = metallicRoughness;
    slot.emissiveMap = emissiveMap;
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
}

bool GltfMaterialLoader::LoadFromCgltf(
        const cgltf_material* mat,
        const char* gltfPath,
        GltfMaterial& outMaterial) {
    if (mat == nullptr) {
        return false;
    }
    outMaterial = GltfMaterial{};
    return TryLoadTexturesFromMaterial(*mat, gltfPath, outMaterial);
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
    outMaterials.Resize(static_cast<std::size_t>(data->materials_count));
    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        (void)LoadFromCgltf(&data->materials[mi], gltfPath, outMaterials[static_cast<std::size_t>(mi)]);
    }
}

}  // namespace Spark
