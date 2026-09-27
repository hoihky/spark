#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TilemapDocument.hpp"

namespace Spark {

class GameObject;
class TilemapComponent;

/** ECS runtime → portable <c>TilemapDocument</c>. */
class TilemapDocumentCapturer final {
public:
    class Options {
    public:
        bool includeObjectLayers = true;
        Utf8String sourceTmxPathOverride{};
    };

    class Result {
    public:
        [[nodiscard]] bool IsSuccess() const noexcept { return success; }
        [[nodiscard]] const Utf8String& GetErrorMessage() const noexcept { return errorMessage; }
        [[nodiscard]] const TilemapDocument& GetDocument() const noexcept { return document; }

        bool success = false;
        Utf8String errorMessage{};
        TilemapDocument document{};
    };

    [[nodiscard]] Result Capture(const GameObject& owner, const TilemapComponent& tilemap) const noexcept;
    [[nodiscard]] Result Capture(
            const GameObject& owner,
            const TilemapComponent& tilemap,
            const Options& options) const noexcept;

    [[nodiscard]] Result CaptureFromOwner(const GameObject& owner) const noexcept;
    [[nodiscard]] Result CaptureFromOwner(const GameObject& owner, const Options& options) const noexcept;
};

}  // namespace Spark
