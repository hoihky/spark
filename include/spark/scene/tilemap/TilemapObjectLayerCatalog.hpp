#pragma once

#include "spark/core/Utf8String.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class TilemapObjectLayerComponent;

/** Read-only view over object markers for spawn / validation (Repository accessor). */
class TilemapObjectLayerCatalog final {
public:
    explicit TilemapObjectLayerCatalog(const TilemapObjectLayerComponent& objectLayers) noexcept;

    [[nodiscard]] std::size_t CountMarkersByTypeId(const char* typeId) const noexcept;
    [[nodiscard]] bool ContainsTypeId(const char* typeId) const noexcept;
    [[nodiscard]] std::size_t TotalMarkerCount() const noexcept;

private:
    const TilemapObjectLayerComponent& layers;
};

}  // namespace Spark
