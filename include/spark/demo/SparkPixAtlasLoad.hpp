#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <cstdint>

namespace Spark {

struct SparkPixAtlasClip {
    Utf8String name{};
    std::uint32_t firstFrame = 0;
    std::uint32_t frameCount = 1;
    float framesPerSecond = 8.0F;
    bool loop = true;
};

/** Parsed <c>*.spark-atlas.json</c> (format <c>spark-sprite-atlas-v1</c>). */
struct SparkPixAtlasManifest {
    Utf8String textureFileName{};
    std::uint32_t columns = 1;
    std::uint32_t rows = 1;
    std::uint32_t frameWidth = 0;
    std::uint32_t frameHeight = 0;
    std::uint32_t padding = 0;
    bool nearestFilter = true;
    Array<SparkPixAtlasClip> clips{};
};

/**
 * Loads manifest JSON from Spark asset roots (see <c>SPARK_ASSETS_DIR</c>, <c>assets/</c>, …).
 * @p manifestRelativePath e.g. <c>/sprites/spark_pix/slime.spark-atlas.json</c>.
 * @p outResolvedManifestPath receives the filesystem path that was read (for texture sibling lookup).
 */
[[nodiscard]] bool TryLoadSparkPixAtlasManifest(
        const char* manifestRelativePath,
        SparkPixAtlasManifest& out,
        Utf8String& outResolvedManifestPath) noexcept;

/** Loads the PNG named in the manifest next to the resolved manifest file. */
[[nodiscard]] bool TryLoadSparkPixAtlasTexture(
        const SparkPixAtlasManifest& manifest,
        const Utf8String& resolvedManifestPath,
        Texture2D& out) noexcept;

/** Configures grid + clips on an existing animator component. */
void ApplySparkPixManifestToAnimator(
        const SparkPixAtlasManifest& manifest,
        SpriteAnimatorComponent& animator) noexcept;

}  // namespace Spark
