#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/texture/TextureFormat.hpp"
#include "spark/scene/texture/TextureLoader.hpp"
#include "spark/scene/texture/TextureMipLevel.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * CPU-side texture asset. Stores RGBA8 or RGBA32F pixels and/or pre-baked block-compressed mip chains
 * (BC7, ASTC 4x4). GPU upload may resample, generate mips, or transcode at scene submit time.
 */
class Texture2D {
public:
    Texture2D() = default;
    explicit Texture2D(Utf8String textureName);

    [[nodiscard]] Utf8String& GetName() noexcept { return name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }

    [[nodiscard]] std::uint32_t GetWidth() const noexcept { return width; }
    [[nodiscard]] std::uint32_t GetHeight() const noexcept { return height; }
    [[nodiscard]] const Array<std::uint8_t>& GetRgba() const noexcept { return rgba; }
    [[nodiscard]] const Array<float>& GetRgbaFloat() const noexcept { return rgbaFloat; }
    [[nodiscard]] TexturePixelFormat GetPixelFormat() const noexcept { return pixelFormat; }
    [[nodiscard]] bool IsHdrFloatPixels() const noexcept { return pixelFormat == TexturePixelFormat::Rgba32Float; }
    [[nodiscard]] bool HasPrebuiltMipChain() const noexcept { return !mipChain.IsEmpty(); }
    [[nodiscard]] const Array<TextureMipLevel>& GetMipChain() const noexcept { return mipChain; }

    void SetPixels(std::uint32_t w, std::uint32_t h, Array<std::uint8_t> bytes);
    void SetFloatPixels(std::uint32_t w, std::uint32_t h, Array<float> pixels);
    void SetCompressedMipChain(TexturePixelFormat format, std::uint32_t w, std::uint32_t h, Array<TextureMipLevel> mips);

    /** Nearest-neighbor resize into outRgba (w*h*4 bytes). */
    void ResampleNearest(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const;
    /** Bilinear resize into outRgba (w*h*4 bytes). */
    void ResampleBilinear(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const;
    /** Bilinear resize into outRgba (w*h*4 floats). */
    void ResampleBilinearFloat(std::uint32_t targetW, std::uint32_t targetH, Array<float>& outRgba) const;

    /** When true, scene upload uses nearest filtering and mip0-only sampling (2D pixel art / atlases). */
    void SetSceneUploadNearest(bool nearest) noexcept { sceneUploadNearest = nearest; }
    [[nodiscard]] bool GetSceneUploadNearest() const noexcept { return sceneUploadNearest; }

    /**
     * When set, scene submit samples <c>atlasSource</c> using <c>atlasUvRect</c> (normalized min/max UV).
     * Mesh UVs are transformed as <c>uv * scale + offset</c> where scale = (max-min) and offset = (minU, minV).
     */
    void SetAtlasBinding(SharedPtr<Texture2D> atlas, Vector4 uvRect) noexcept;
    void ClearAtlasBinding() noexcept;
    [[nodiscard]] bool HasAtlasBinding() const noexcept { return static_cast<bool>(atlasSource); }
    [[nodiscard]] SharedPtr<Texture2D> ResolveAtlasUv(Vector2& outScale, Vector2& outOffset) const noexcept;

    /**
     * Fraction of the GPU scene layer (0–1) occupied by uploaded content after uniform aspect-preserving fit.
     * Sprite/tilemap UV rects in source-image space must be multiplied by this before sampling.
     */
    [[nodiscard]] Vector2 GetSceneLayerUvScale() const noexcept { return sceneLayerUvScale; }
    [[nodiscard]] Vector4 ScaleUvRectForSceneLayer(const Vector4& uv) const noexcept;

    /** Uniform-fit into a square scene layer; updates <c>sceneLayerUvScale</c>. */
    void PrepareSceneLayerUpload(std::uint32_t layerSize, Array<std::uint8_t>& outRgba);
    /** Uniform-fit HDR float pixels into a square scene layer; updates <c>sceneLayerUvScale</c>. */
    void PrepareSceneLayerUploadFloat(std::uint32_t layerSize, Array<float>& outRgba);

    /** Stable GPU array layer for this texture (assigned once on first scene submit). */
    [[nodiscard]] std::int32_t GetGpuSceneLayer() const noexcept { return gpuSceneLayer; }
    void EnsureGpuSceneLayer(std::int32_t& nextFreeLayer) noexcept;

    /** Cached FNV-1a fingerprint; refreshed when pixels or mips change. */
    [[nodiscard]] std::uint64_t GetContentFingerprint() const noexcept { return contentFingerprint; }

    static Texture2D CreateCheckerboard(
            std::uint32_t size,
            std::uint32_t tilePixels,
            Vector3 colorA,
            Vector3 colorB);

    static Texture2D CreateSolid(std::uint32_t w, std::uint32_t h, Vector3 rgb, float alpha = 1.0F);

    /**
     * Load PNG/JPEG/etc. via stb_image into RGBA8. Also accepts raw KTX2 files with BC7/ASTC payloads.
     * Returns false if the file is missing or invalid.
     */
    [[nodiscard]] static bool TryLoadFromFile(const char* path, Texture2D& out, bool flipVerticalOnLoad = true);

    /** Decode embedded image bytes (e.g. from glTF buffer views). */
    [[nodiscard]] static bool TryLoadFromMemory(
            const std::uint8_t* bytes, std::size_t byteCount, Texture2D& out, const char* debugName = "Memory");

    /** Load a KTX2 file with uncompressed BC7 or ASTC 4x4 mip data (supercompression scheme 0). */
    [[nodiscard]] static bool TryLoadFromKtx2File(const char* path, Texture2D& out);

    static Texture2D CreateBrickPattern(std::uint32_t width, std::uint32_t height);
    static Texture2D CreateSoilPattern(std::uint32_t width, std::uint32_t height);
    static Texture2D CreateVariedSoilGroundPattern(std::uint32_t width, std::uint32_t height);

private:
    Utf8String name;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    Array<std::uint8_t> rgba;
    Array<float> rgbaFloat;
    TexturePixelFormat pixelFormat = TexturePixelFormat::Rgba8Unorm;
    Array<TextureMipLevel> mipChain;
    std::uint64_t contentFingerprint = 0;
    bool sceneUploadNearest = false;
    Vector2 sceneLayerUvScale{1.0F, 1.0F};
    SharedPtr<Texture2D> atlasSource;
    Vector4 atlasUvRect{0.0F, 0.0F, 1.0F, 1.0F};
    std::int32_t gpuSceneLayer = -1;

    void RefreshContentFingerprint() noexcept;
    void RefreshSceneLayerUvScale() noexcept;
};

}  // namespace Spark
