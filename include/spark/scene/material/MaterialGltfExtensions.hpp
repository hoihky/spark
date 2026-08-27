#pragma once

namespace Spark {

/**
 * glTF KHR material extensions carried through ECS → submit → GPU push constants.
 * Defaults match spec when an extension is absent (no effect on legacy materials).
 */
struct MaterialGltfExtensions {
    /** KHR_materials_clearcoat clearcoatFactor (0 = disabled). */
    float clearcoatFactor = 0.0F;
    /** KHR_materials_clearcoat clearcoatRoughnessFactor. */
    float clearcoatRoughnessFactor = 0.0F;
    /** KHR_materials_transmission transmissionFactor (0 = opaque). */
    float transmissionFactor = 0.0F;
    /** KHR_materials_emissive_strength multiplier (default 1). */
    float emissiveStrength = 1.0F;
    /** KHR_materials_iridescence iridescenceFactor (0 = disabled). */
    float iridescenceFactor = 0.0F;
    /** KHR_materials_iridescence iridescenceIor. */
    float iridescenceIor = 1.3F;
    /** KHR_materials_iridescence thickness range in nanometers. */
    float iridescenceThicknessMin = 100.0F;
    float iridescenceThicknessMax = 400.0F;

    [[nodiscard]] bool HasClearcoat() const noexcept { return clearcoatFactor > 1.0e-6F; }
    [[nodiscard]] bool HasTransmission() const noexcept { return transmissionFactor > 1.0e-6F; }
    [[nodiscard]] bool HasEmissiveStrength() const noexcept { return emissiveStrength > 1.0F + 1.0e-6F; }
    [[nodiscard]] bool HasIridescence() const noexcept { return iridescenceFactor > 1.0e-6F; }
};

}  // namespace Spark
