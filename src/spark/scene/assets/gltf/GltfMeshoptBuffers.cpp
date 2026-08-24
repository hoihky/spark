#include "spark/scene/assets/gltf/GltfMeshoptBuffers.hpp"

#include "cgltf.h"

#if SPARK_ENABLE_GLTF_MESHOPT
#include <meshoptimizer.h>
#endif

namespace Spark {

namespace {

const unsigned char* MeshoptSourceBytes(const cgltf_meshopt_compression& compression) {
    if (compression.buffer == nullptr || compression.buffer->data == nullptr) {
        return nullptr;
    }
    return static_cast<const unsigned char*>(compression.buffer->data) + compression.offset;
}

bool HasUncompressedFallback(
        const cgltf_buffer_view& view,
        const cgltf_meshopt_compression& compression) noexcept {
    if (view.buffer == nullptr || view.buffer->data == nullptr) {
        return false;
    }
    if (view.buffer == compression.buffer) {
        return false;
    }
    return view.offset + view.size <= view.buffer->size;
}

}  // namespace

bool GltfMeshoptBuffers::TryDecodeAll(cgltf_data* data, Utf8String& outErrorMessage) noexcept {
    if (data == nullptr) {
        outErrorMessage = Utf8String("Null glTF data");
        return false;
    }

#if !SPARK_ENABLE_GLTF_MESHOPT
    for (cgltf_size i = 0; i < data->buffer_views_count; ++i) {
        if (data->buffer_views[i].has_meshopt_compression) {
            outErrorMessage = Utf8String(
                    "glTF uses meshopt compression; rebuild Spark with SPARK_ENABLE_GLTF_MESHOPT=ON "
                    "or re-export without meshopt");
            return false;
        }
    }
    return true;
#else
    for (cgltf_size i = 0; i < data->buffer_views_count; ++i) {
        cgltf_buffer_view& view = data->buffer_views[i];
        if (!view.has_meshopt_compression || view.data != nullptr) {
            continue;
        }

        const cgltf_meshopt_compression& compression = view.meshopt_compression;
        if (HasUncompressedFallback(view, compression)) {
            continue;
        }
        const unsigned char* source = MeshoptSourceBytes(compression);
        if (source == nullptr) {
            outErrorMessage = Utf8String("Meshopt compression buffer is missing or unreadable");
            return false;
        }

        const std::size_t outputSize = static_cast<std::size_t>(compression.count * compression.stride);
        void* decoded = data->memory.alloc_func(data->memory.user_data, outputSize);
        if (decoded == nullptr) {
            outErrorMessage = Utf8String("Out of memory decompressing meshopt buffer view");
            return false;
        }

        int decodeResult = -1;
        switch (compression.mode) {
            case cgltf_meshopt_compression_mode_attributes:
                decodeResult = meshopt_decodeVertexBuffer(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride),
                        source,
                        static_cast<std::size_t>(compression.size));
                break;
            case cgltf_meshopt_compression_mode_triangles:
                decodeResult = meshopt_decodeIndexBuffer(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride),
                        source,
                        static_cast<std::size_t>(compression.size));
                break;
            case cgltf_meshopt_compression_mode_indices:
                decodeResult = meshopt_decodeIndexSequence(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride),
                        source,
                        static_cast<std::size_t>(compression.size));
                break;
            default:
                data->memory.free_func(data->memory.user_data, decoded);
                outErrorMessage = Utf8String("Unsupported meshopt compression mode");
                return false;
        }

        if (decodeResult != 0) {
            data->memory.free_func(data->memory.user_data, decoded);
            outErrorMessage = Utf8String("Meshopt decompression failed");
            return false;
        }

        switch (compression.filter) {
            case cgltf_meshopt_compression_filter_none:
                break;
            case cgltf_meshopt_compression_filter_octahedral:
                meshopt_decodeFilterOct(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride));
                break;
            case cgltf_meshopt_compression_filter_quaternion:
                meshopt_decodeFilterQuat(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride));
                break;
            case cgltf_meshopt_compression_filter_exponential:
                meshopt_decodeFilterExp(
                        decoded,
                        static_cast<std::size_t>(compression.count),
                        static_cast<std::size_t>(compression.stride));
                break;
            case cgltf_meshopt_compression_filter_color:
                // meshoptimizer 0.22 has no color filter decoder; raw values are usable as-is.
                break;
            default:
                data->memory.free_func(data->memory.user_data, decoded);
                outErrorMessage = Utf8String("Unsupported meshopt compression filter");
                return false;
        }

        view.data = decoded;
    }

    return true;
#endif
}

}  // namespace Spark
