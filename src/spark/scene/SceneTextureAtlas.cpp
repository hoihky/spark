#include "spark/scene/SceneTextureAtlas.hpp"

#include "spark/scene/Texture2D.hpp"

#include <algorithm>
#include <cstring>

namespace Spark {

SceneTextureAtlas::SceneTextureAtlas(const std::uint32_t inAtlasSize, const std::uint32_t inPadding)
    : atlasSize(inAtlasSize), padding(inPadding) {
    atlasPixels.Resize(static_cast<std::size_t>(atlasSize) * static_cast<std::size_t>(atlasSize) * 4U);
    for (std::size_t i = 0; i < atlasPixels.GetSize(); ++i) {
        atlasPixels[i] = 0;
    }
}

bool SceneTextureAtlas::TryAdd(const SharedPtr<Texture2D>& texture) {
    if (!texture || texture->GetWidth() == 0 || texture->GetHeight() == 0) {
        return false;
    }
    if (placements.Find(texture.Get()) != nullptr) {
        return false;
    }
    const std::uint32_t texW = texture->GetWidth();
    const std::uint32_t texH = texture->GetHeight();
    const std::uint32_t slotW = texW + padding * 2U;
    const std::uint32_t slotH = texH + padding * 2U;
    if (slotW > atlasSize || slotH > atlasSize) {
        return false;
    }

    if (cursorX + slotW > atlasSize) {
        cursorX = 0;
        rowY += rowHeight;
        rowHeight = 0;
    }
    if (rowY + slotH > atlasSize) {
        return false;
    }

    Placement placement{};
    placement.pixelX = cursorX + padding;
    placement.pixelY = rowY + padding;
    placement.pixelW = texW;
    placement.pixelH = texH;
    const float inv = 1.0F / static_cast<float>(atlasSize);
    placement.uvRect = Vector4{
            static_cast<float>(placement.pixelX) * inv,
            static_cast<float>(placement.pixelY) * inv,
            static_cast<float>(placement.pixelX + placement.pixelW) * inv,
            static_cast<float>(placement.pixelY + placement.pixelH) * inv};

    const Array<std::uint8_t>& src = texture->GetRgba();
    for (std::uint32_t y = 0; y < texH; ++y) {
        const std::size_t dstRow =
                (static_cast<std::size_t>(placement.pixelY + y) * static_cast<std::size_t>(atlasSize) +
                 static_cast<std::size_t>(placement.pixelX)) *
                4U;
        const std::size_t srcRow = static_cast<std::size_t>(y) * static_cast<std::size_t>(texW) * 4U;
        std::memcpy(
                atlasPixels.GetData() + dstRow,
                src.GetData() + srcRow,
                static_cast<std::size_t>(texW) * 4U);
    }

    placements.Add(texture.Get(), placement);
    sources.PushBack(texture);
    cursorX += slotW;
    rowHeight = std::max(rowHeight, slotH);
    return true;
}

SharedPtr<Texture2D> SceneTextureAtlas::Finalize(Utf8String atlasName) {
    if (sources.IsEmpty()) {
        return SharedPtr<Texture2D>();
    }
    auto atlas = MakeShared<Texture2D>(MoveTemp(atlasName));
    atlas->SetPixels(atlasSize, atlasSize, MoveTemp(atlasPixels));
    atlas->SetSceneUploadNearest(true);
    atlasPixels.Clear();

    for (std::size_t i = 0; i < sources.GetSize(); ++i) {
        const SharedPtr<Texture2D>& source = sources[i];
        if (!source) {
            continue;
        }
        if (const Placement* placement = placements.Find(source.Get())) {
            source->SetAtlasBinding(atlas, placement->uvRect);
        }
    }
    return atlas;
}

const SceneTextureAtlas::Placement* SceneTextureAtlas::TryGetPlacement(const Texture2D* texture) const noexcept {
    if (texture == nullptr) {
        return nullptr;
    }
    return placements.Find(texture);
}

}  // namespace Spark
