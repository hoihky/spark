#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"

namespace Spark {

class GameObject;
class GameWorld;

struct TilemapDocumentApplyOptions {
    /** Pixels per world unit when deriving <c>tileWorldSize</c> from Tiled tile size. */
    float pixelsPerWorldUnit = 16.0F;
    /** When false, only updates cells on an existing <c>TilemapComponent</c>. */
    bool createComponentsIfMissing = true;
    /** Apply object layers to <c>TilemapObjectLayerComponent</c> when present / created. */
    bool applyObjectLayers = true;
};

struct TilemapDocumentApplyResult {
    bool success = false;
    Utf8String errorMessage{};
};

/**
 * Builds or updates <c>TilemapComponent</c> (+ optional object layers) on <c>owner</c> from a document.
 * Loads the primary tileset image through <c>GameWorld::LoadTexture</c>.
 */
[[nodiscard]] TilemapDocumentApplyResult ApplyTilemapDocument(
        const TilemapDocument& document,
        GameObject& owner,
        GameWorld& world,
        const TilemapDocumentApplyOptions& options = {});

}  // namespace Spark
