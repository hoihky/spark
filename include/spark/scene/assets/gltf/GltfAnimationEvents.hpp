#pragma once

struct cgltf_data;

namespace Spark {

class Skeleton;

/**
 * Loads Spark animation events into a <c>Skeleton</c> from:
 * - Sidecar <c>{model}.spark-anim-events.json</c> next to the glTF file
 *
 * glTF animation <c>extras.sparkEvents</c> is reserved for a future cgltf extras hook.
 */
void LoadGltfAnimationEvents(cgltf_data* data, const char* gltfPath, Skeleton& skeleton) noexcept;

}  // namespace Spark
