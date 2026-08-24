#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"

#include "cgltf.h"

#if SPARK_ENABLE_GLTF_DRACO
#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/mesh/mesh.h"
#endif

namespace Spark {

namespace {

#if SPARK_ENABLE_GLTF_DRACO

const draco::PointAttribute* FindDracoAttribute(
        const draco::Mesh& mesh,
        const cgltf_data* data,
        const cgltf_draco_mesh_compression& dracoExt,
        const cgltf_attribute_type type,
        const cgltf_int setIndex) {
    for (cgltf_size ai = 0; ai < dracoExt.attributes_count; ++ai) {
        const cgltf_attribute& mapped = dracoExt.attributes[ai];
        if (mapped.type != type || mapped.index != setIndex) {
            continue;
        }
        const std::int32_t uniqueId = GltfDracoAttributeUniqueId(data, mapped);
        if (uniqueId < 0) {
            return nullptr;
        }
        return mesh.GetAttributeByUniqueId(static_cast<int>(uniqueId));
    }
    return nullptr;
}

bool ReadAttributeVec3(
        const draco::PointAttribute* attribute,
        const draco::PointIndex pointIndex,
        Vector3& outValue) {
    if (attribute == nullptr || attribute->num_components() < 3) {
        return false;
    }
    float values[3]{};
    if (!attribute->ConvertValue<float>(attribute->mapped_index(pointIndex), values)) {
        return false;
    }
    outValue = {values[0], values[1], values[2]};
    return true;
}

bool ReadAttributeVec2(
        const draco::PointAttribute* attribute,
        const draco::PointIndex pointIndex,
        Vector2& outValue) {
    if (attribute == nullptr || attribute->num_components() < 2) {
        return false;
    }
    float values[2]{};
    if (!attribute->ConvertValue<float>(attribute->mapped_index(pointIndex), values)) {
        return false;
    }
    outValue = {values[0], values[1]};
    return true;
}

bool ReadAttributeVec4(
        const draco::PointAttribute* attribute,
        const draco::PointIndex pointIndex,
        Vector4& outValue) {
    if (attribute == nullptr || attribute->num_components() < 4) {
        return false;
    }
    float values[4]{};
    if (!attribute->ConvertValue<float>(attribute->mapped_index(pointIndex), values)) {
        return false;
    }
    outValue = {values[0], values[1], values[2], values[3]};
    return true;
}

bool ReadAttributeUInt32(
        const draco::PointAttribute* attribute,
        const draco::PointIndex pointIndex,
        const int componentCount,
        std::uint32_t* outValues) {
    if (attribute == nullptr || componentCount <= 0) {
        return false;
    }
    for (int ci = 0; ci < componentCount; ++ci) {
        draco::AttributeValueIndex valueIndex = attribute->mapped_index(pointIndex);
        if (attribute->num_components() == 1) {
            valueIndex = draco::AttributeValueIndex(valueIndex.value() + static_cast<uint32_t>(ci));
        }
        uint32_t value = 0;
        if (!attribute->ConvertValue<uint32_t>(valueIndex, &value)) {
            return false;
        }
        outValues[ci] = value;
    }
    return true;
}

GltfDecodedSkinning NormalizeSkinning(const std::uint32_t joints[4], const float weights[4]) {
    GltfDecodedSkinning skinning{};
    for (int i = 0; i < 4; ++i) {
        skinning.joints[i] = joints[i];
        skinning.weights[i] = weights[i];
    }
    const float sum = skinning.weights[0] + skinning.weights[1] + skinning.weights[2] + skinning.weights[3];
    if (sum > 1.0e-6F) {
        const float inv = 1.0F / sum;
        skinning.weights[0] *= inv;
        skinning.weights[1] *= inv;
        skinning.weights[2] *= inv;
        skinning.weights[3] *= inv;
    } else {
        skinning.weights[0] = 1.0F;
        skinning.weights[1] = skinning.weights[2] = skinning.weights[3] = 0.0F;
    }
    return skinning;
}

#endif

class GltfDracoPrimitiveDecoder final : public IGltfPrimitiveDecoder {
public:
    [[nodiscard]] bool Supports(const cgltf_primitive* prim) const noexcept override {
        return prim != nullptr && prim->has_draco_mesh_compression;
    }

    [[nodiscard]] GltfPrimitiveDecodeResult Decode(
            const cgltf_data* data,
            const cgltf_primitive* prim,
            const GltfPrimitiveDecodeOptions& options) const override {
        GltfPrimitiveDecodeResult result{};
        if (prim == nullptr || !prim->has_draco_mesh_compression) {
            result.errorMessage = Utf8String("Primitive is not Draco-compressed");
            return result;
        }
        if (prim->type != cgltf_primitive_type_triangles) {
            result.errorMessage = Utf8String("Draco primitive must be triangles");
            return result;
        }

#if !SPARK_ENABLE_GLTF_DRACO
        (void)data;
        (void)options;
        result.errorMessage = Utf8String(
                "KHR_draco_mesh_compression is present; rebuild Spark with SPARK_ENABLE_GLTF_DRACO=ON "
                "or re-export without Draco");
        return result;
#else
        const cgltf_draco_mesh_compression& dracoExt = prim->draco_mesh_compression;
        const cgltf_buffer_view* bufferView = dracoExt.buffer_view;
        if (bufferView == nullptr || bufferView->buffer == nullptr || bufferView->buffer->data == nullptr) {
            result.errorMessage = Utf8String("Draco buffer view is missing or unreadable");
            return result;
        }

        const auto* compressedBytes =
                static_cast<const char*>(bufferView->buffer->data) + bufferView->offset;
        const size_t compressedSize = static_cast<size_t>(bufferView->size);

        draco::DecoderBuffer decoderBuffer;
        decoderBuffer.Init(compressedBytes, compressedSize);

        draco::Decoder decoder;
        const draco::StatusOr<std::unique_ptr<draco::Mesh>> decoded = decoder.DecodeMeshFromBuffer(&decoderBuffer);
        if (!decoded.ok()) {
            result.errorMessage = Utf8String("Draco decode failed: ");
            if (decoded.status().error_msg() != nullptr) {
                result.errorMessage.AppendUtf8(decoded.status().error_msg());
            }
            return result;
        }

        const draco::Mesh& mesh = *decoded.value();
        if (mesh.num_points() == 0) {
            result.errorMessage = Utf8String("Draco mesh has no vertices");
            return result;
        }

        const draco::PointAttribute* const positionAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_position, 0);
        if (positionAttribute == nullptr) {
            result.errorMessage = Utf8String("Draco mesh is missing POSITION attribute");
            return result;
        }

        const draco::PointAttribute* const normalAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_normal, 0);
        const draco::PointAttribute* const tangentAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_tangent, 0);
        const draco::PointAttribute* const jointsAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_joints, 0);
        const draco::PointAttribute* const weightsAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_weights, 0);
        const draco::PointAttribute* const texcoordAttribute = [&]() -> const draco::PointAttribute* {
            const draco::PointAttribute* selected =
                    FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_texcoord, static_cast<cgltf_int>(options.texCoordSet));
            if (selected != nullptr) {
                return selected;
            }
            return FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_texcoord, 0);
        }();
        const draco::PointAttribute* const texcoord1Attribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_texcoord, 1);
        const draco::PointAttribute* const colorAttribute =
                FindDracoAttribute(mesh, data, dracoExt, cgltf_attribute_type_color, 0);

        const cgltf_size vertexCount = mesh.num_points();
        result.primitive.positions.Resize(vertexCount);
        if (normalAttribute != nullptr) {
            result.primitive.normals.Resize(vertexCount);
        }
        if (texcoordAttribute != nullptr) {
            result.primitive.texcoords.Resize(vertexCount);
        }
        if (texcoord1Attribute != nullptr) {
            result.primitive.texcoords1.Resize(vertexCount);
        }
        if (colorAttribute != nullptr) {
            result.primitive.colors.Resize(vertexCount);
        }
        if (tangentAttribute != nullptr) {
            result.primitive.tangents.Resize(vertexCount);
        }
        if (jointsAttribute != nullptr && weightsAttribute != nullptr) {
            result.primitive.skinning.Resize(vertexCount);
        }

        for (draco::PointIndex pointIndex(0); pointIndex < mesh.num_points(); ++pointIndex) {
            const cgltf_size outIndex = static_cast<cgltf_size>(pointIndex.value());
            if (!ReadAttributeVec3(positionAttribute, pointIndex, result.primitive.positions[outIndex])) {
                result.errorMessage = Utf8String("Failed to read Draco POSITION");
                return result;
            }
            if (normalAttribute != nullptr) {
                if (!ReadAttributeVec3(normalAttribute, pointIndex, result.primitive.normals[outIndex])) {
                    result.errorMessage = Utf8String("Failed to read Draco NORMAL");
                    return result;
                }
            }
            if (texcoordAttribute != nullptr) {
                if (!ReadAttributeVec2(texcoordAttribute, pointIndex, result.primitive.texcoords[outIndex])) {
                    result.errorMessage = Utf8String("Failed to read Draco TEXCOORD");
                    return result;
                }
            }
            if (texcoord1Attribute != nullptr) {
                if (!ReadAttributeVec2(texcoord1Attribute, pointIndex, result.primitive.texcoords1[outIndex])) {
                    result.errorMessage = Utf8String("Failed to read Draco TEXCOORD_1");
                    return result;
                }
            }
            if (colorAttribute != nullptr) {
                if (colorAttribute->num_components() >= 4) {
                    if (!ReadAttributeVec4(colorAttribute, pointIndex, result.primitive.colors[outIndex])) {
                        result.errorMessage = Utf8String("Failed to read Draco COLOR");
                        return result;
                    }
                } else {
                    Vector3 rgb{};
                    if (!ReadAttributeVec3(colorAttribute, pointIndex, rgb)) {
                        result.errorMessage = Utf8String("Failed to read Draco COLOR");
                        return result;
                    }
                    result.primitive.colors[outIndex] = {rgb.x, rgb.y, rgb.z, 1.0F};
                }
            }
            if (tangentAttribute != nullptr) {
                if (!ReadAttributeVec4(tangentAttribute, pointIndex, result.primitive.tangents[outIndex])) {
                    result.errorMessage = Utf8String("Failed to read Draco TANGENT");
                    return result;
                }
            }
            if (!result.primitive.skinning.IsEmpty()) {
                std::uint32_t joints[4]{};
                float weights[4]{1.0F, 0.0F, 0.0F, 0.0F};
                if (!ReadAttributeUInt32(jointsAttribute, pointIndex, 4, joints)) {
                    result.errorMessage = Utf8String("Failed to read Draco JOINTS_0");
                    return result;
                }
                if (!weightsAttribute->ConvertValue<float>(weightsAttribute->mapped_index(pointIndex), weights) ||
                    weightsAttribute->num_components() < 4) {
                    result.errorMessage = Utf8String("Failed to read Draco WEIGHTS_0");
                    return result;
                }
                result.primitive.skinning[outIndex] = NormalizeSkinning(joints, weights);
            }
        }

        if (mesh.num_faces() > 0) {
            result.primitive.indices.Resize(mesh.num_faces() * 3);
            for (draco::FaceIndex faceIndex(0); faceIndex < mesh.num_faces(); ++faceIndex) {
                const draco::Mesh::Face& face = mesh.face(faceIndex);
                const cgltf_size base = static_cast<cgltf_size>(faceIndex.value()) * 3;
                result.primitive.indices[base + 0] = face[0].value();
                result.primitive.indices[base + 1] = face[1].value();
                result.primitive.indices[base + 2] = face[2].value();
            }
        }

        result.ok = true;
        return result;
#endif
    }
};

const GltfDracoPrimitiveDecoder& DracoDecoder() noexcept {
    static const GltfDracoPrimitiveDecoder decoder{};
    return decoder;
}

}  // namespace

const IGltfPrimitiveDecoder& GltfDracoPrimitiveDecoderInstance() noexcept {
    return DracoDecoder();
}

}  // namespace Spark
