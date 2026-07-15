#pragma once

namespace Spark {

/** Half-extent (XZ) of the demo ground plane; must match VulkanRenderer::CreateSceneGeometry. */
inline constexpr float kSceneGroundHalfExtent = 32.0F;
/** World units per texture repeat on the built-in ground plane (Kenney 64px tiles). */
inline constexpr float kSceneGroundWorldUnitsPerTextureRepeat = 2.0F;

}  // namespace Spark
