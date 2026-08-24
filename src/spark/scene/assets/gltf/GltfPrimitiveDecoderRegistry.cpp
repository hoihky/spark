#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"

#include "cgltf.h"

namespace Spark {

const IGltfPrimitiveDecoder& GltfUncompressedPrimitiveDecoderInstance() noexcept;
const IGltfPrimitiveDecoder& GltfDracoPrimitiveDecoderInstance() noexcept;

std::int32_t GltfDracoAttributeUniqueId(const cgltf_data* data, const cgltf_attribute& attr) noexcept {
    if (data == nullptr || attr.data == nullptr || data->accessors_count == 0) {
        return -1;
    }
    const std::ptrdiff_t offset = attr.data - data->accessors;
    if (offset < 0 || static_cast<cgltf_size>(offset) >= data->accessors_count) {
        return -1;
    }
    return static_cast<std::int32_t>(offset);
}

GltfPrimitiveDecodeResult GltfPrimitiveDecoderRegistry::Decode(
        const cgltf_data* data,
        const cgltf_primitive* prim,
        const GltfPrimitiveDecodeOptions& options) {
    GltfPrimitiveDecodeResult result{};
    if (prim == nullptr) {
        result.errorMessage = Utf8String("Null glTF primitive");
        return result;
    }

    const IGltfPrimitiveDecoder* const decoders[] = {
            &GltfDracoPrimitiveDecoderInstance(),
            &GltfUncompressedPrimitiveDecoderInstance(),
    };

    for (const IGltfPrimitiveDecoder* decoder : decoders) {
        if (decoder == nullptr || !decoder->Supports(prim)) {
            continue;
        }
        return decoder->Decode(data, prim, options);
    }

    result.errorMessage = Utf8String("No glTF primitive decoder supports this primitive");
    return result;
}

}  // namespace Spark
