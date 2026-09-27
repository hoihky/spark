#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/** Applies a <c>TilemapDocument</c> onto ECS components (tilemap, object layers, tileset). */
class TilemapDocumentApplier final {
public:
    class Options {
    public:
        float pixelsPerWorldUnit = 16.0F;
        bool createComponentsIfMissing = true;
        bool applyObjectLayers = true;
    };

    class Result {
    public:
        [[nodiscard]] bool IsSuccess() const noexcept { return success; }
        [[nodiscard]] const Utf8String& GetErrorMessage() const noexcept { return errorMessage; }

        bool success = false;
        Utf8String errorMessage{};
    };

    [[nodiscard]] Result Apply(const TilemapDocument& document, GameObject& owner, GameWorld& world) const;
    [[nodiscard]] Result Apply(
            const TilemapDocument& document,
            GameObject& owner,
            GameWorld& world,
            const Options& options) const;
};

}  // namespace Spark
