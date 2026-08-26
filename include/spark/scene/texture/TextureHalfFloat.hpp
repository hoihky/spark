#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/** IEEE-754 float32 → float16 conversion for GPU R16G16B16A16 staging. */
class TextureHalfFloat {
public:
    [[nodiscard]] static std::uint16_t FromFloat32(float value) noexcept;
    static void PackRgba16Float(const float* srcRgba, std::uint16_t* dstRgba, std::size_t pixelCount) noexcept;
};

}  // namespace Spark
