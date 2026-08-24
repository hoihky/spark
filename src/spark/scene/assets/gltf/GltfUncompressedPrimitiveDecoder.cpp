#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"

#include "cgltf.h"

namespace Spark {

namespace {

GltfDecodedSkinning ReadSkinningFromAccessors(
        const cgltf_accessor* joints,
        const cgltf_accessor* weights,
        const cgltf_size vertexIndex) {
    GltfDecodedSkinning skinning{};
    if (joints != nullptr) {
        cgltf_uint jointValues[4]{};
        if (!cgltf_accessor_read_uint(joints, vertexIndex, jointValues, 4)) {
            jointValues[0] = jointValues[1] = jointValues[2] = jointValues[3] = 0;
        }
        skinning.joints[0] = static_cast<std::uint32_t>(jointValues[0]);
        skinning.joints[1] = static_cast<std::uint32_t>(jointValues[1]);
        skinning.joints[2] = static_cast<std::uint32_t>(jointValues[2]);
        skinning.joints[3] = static_cast<std::uint32_t>(jointValues[3]);
    }
    if (weights != nullptr) {
        float weightValues[4]{1.0F, 0.0F, 0.0F, 0.0F};
        cgltf_accessor_read_float(weights, vertexIndex, weightValues, 4);
        float sum = weightValues[0] + weightValues[1] + weightValues[2] + weightValues[3];
        if (sum > 1.0e-6F) {
            const float inv = 1.0F / sum;
            weightValues[0] *= inv;
            weightValues[1] *= inv;
            weightValues[2] *= inv;
            weightValues[3] *= inv;
        } else {
            weightValues[0] = 1.0F;
            weightValues[1] = weightValues[2] = weightValues[3] = 0.0F;
        }
        skinning.weights[0] = weightValues[0];
        skinning.weights[1] = weightValues[1];
        skinning.weights[2] = weightValues[2];
        skinning.weights[3] = weightValues[3];
    }
    return skinning;
}

const cgltf_accessor* FindAccessor(
        const cgltf_primitive* prim,
        const cgltf_attribute_type type,
        const cgltf_int setIndex) {
    if (prim == nullptr) {
        return nullptr;
    }
    for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai) {
        const cgltf_attribute& attribute = prim->attributes[ai];
        if (attribute.type == type && attribute.index == setIndex) {
            return attribute.data;
        }
    }
    return nullptr;
}

}  // namespace

class GltfUncompressedPrimitiveDecoder final : public IGltfPrimitiveDecoder {
public:
    [[nodiscard]] bool Supports(const cgltf_primitive* prim) const noexcept override {
        return prim != nullptr && !prim->has_draco_mesh_compression;
    }

    [[nodiscard]] GltfPrimitiveDecodeResult Decode(
            const cgltf_data* /*data*/,
            const cgltf_primitive* prim,
            const GltfPrimitiveDecodeOptions& options) const override {
        GltfPrimitiveDecodeResult result{};
        if (prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
            result.errorMessage = Utf8String("Unsupported glTF primitive type");
            return result;
        }

        const cgltf_accessor* pos = FindAccessor(prim, cgltf_attribute_type_position, 0);
        const cgltf_accessor* nrm = FindAccessor(prim, cgltf_attribute_type_normal, 0);
        const cgltf_accessor* tan = FindAccessor(prim, cgltf_attribute_type_tangent, 0);
        const cgltf_accessor* joints = FindAccessor(prim, cgltf_attribute_type_joints, 0);
        const cgltf_accessor* weights = FindAccessor(prim, cgltf_attribute_type_weights, 0);
        const cgltf_accessor* uv = FindAccessor(prim, cgltf_attribute_type_texcoord, static_cast<cgltf_int>(options.texCoordSet));
        if (uv == nullptr && options.texCoordSet != 0) {
            uv = FindAccessor(prim, cgltf_attribute_type_texcoord, 0);
        }

        if (pos == nullptr || pos->type != cgltf_type_vec3) {
            result.errorMessage = Utf8String("glTF primitive is missing POSITION attribute");
            return result;
        }

        const cgltf_size vertexCount = pos->count;
        result.primitive.positions.Resize(vertexCount);
        if (nrm != nullptr && nrm->type == cgltf_type_vec3) {
            result.primitive.normals.Resize(vertexCount);
        }
        if (uv != nullptr && uv->type == cgltf_type_vec2) {
            result.primitive.texcoords.Resize(vertexCount);
        }
        if (tan != nullptr && tan->type == cgltf_type_vec4) {
            result.primitive.tangents.Resize(vertexCount);
        }
        if (joints != nullptr && weights != nullptr) {
            result.primitive.skinning.Resize(vertexCount);
        }

        for (cgltf_size vi = 0; vi < vertexCount; ++vi) {
            float position[3]{};
            cgltf_accessor_read_float(pos, vi, position, 3);
            result.primitive.positions[vi] = {position[0], position[1], position[2]};

            if (!result.primitive.normals.IsEmpty()) {
                float normal[3]{};
                cgltf_accessor_read_float(nrm, vi, normal, 3);
                result.primitive.normals[vi] = {normal[0], normal[1], normal[2]};
            }
            if (!result.primitive.texcoords.IsEmpty()) {
                float texcoord[2]{};
                cgltf_accessor_read_float(uv, vi, texcoord, 2);
                result.primitive.texcoords[vi] = {texcoord[0], texcoord[1]};
            }
            if (!result.primitive.tangents.IsEmpty()) {
                float tangent[4]{};
                cgltf_accessor_read_float(tan, vi, tangent, 4);
                result.primitive.tangents[vi] = {tangent[0], tangent[1], tangent[2], tangent[3]};
            }
            if (!result.primitive.skinning.IsEmpty()) {
                result.primitive.skinning[vi] = ReadSkinningFromAccessors(joints, weights, vi);
            }
        }

        if (prim->indices != nullptr) {
            const cgltf_accessor* indices = prim->indices;
            result.primitive.indices.Resize(indices->count);
            for (cgltf_size ii = 0; ii < indices->count; ++ii) {
                result.primitive.indices[ii] =
                        static_cast<std::uint32_t>(cgltf_accessor_read_index(indices, ii));
            }
        }

        result.ok = true;
        return result;
    }
};

const GltfUncompressedPrimitiveDecoder& UncompressedDecoder() noexcept {
    static const GltfUncompressedPrimitiveDecoder decoder{};
    return decoder;
}

const IGltfPrimitiveDecoder& GltfUncompressedPrimitiveDecoderInstance() noexcept {
    return UncompressedDecoder();
}

}  // namespace Spark
