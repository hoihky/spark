#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/RenderLayerId.hpp"

namespace Spark {

/** Named sorting layer with a global relative draw order (lower = drawn earlier / behind). */
struct RenderLayerDescriptor {
    Utf8String name{};
    std::int16_t sortingOrder = 0;
};

/**
 * Project-wide table of named render layers (Unity-style Sorting Layers).
 * Lower <c>sortingOrder</c> values are submitted and rasterized before higher ones within a blend pass.
 */
class RenderLayerRegistry {
public:
    [[nodiscard]] static RenderLayerRegistry& Instance() noexcept;

    /** Registers a layer or returns the existing id when <c>name</c> already exists. */
    [[nodiscard]] RenderLayerId RegisterLayer(const char* name, std::int16_t sortingOrder);

    [[nodiscard]] RenderLayerId FindLayerIdByName(const char* name) const noexcept;
    [[nodiscard]] std::int16_t GetLayerSortingOrder(RenderLayerId id) const noexcept;
    [[nodiscard]] const char* GetLayerName(RenderLayerId id) const noexcept;

    [[nodiscard]] RenderLayerId GetDefaultLayerId() const noexcept { return defaultLayerId_; }
    [[nodiscard]] std::int16_t GetDefaultLayerSortingOrder() const noexcept;

    [[nodiscard]] std::size_t GetLayerCount() const noexcept { return layers_.GetSize(); }
    [[nodiscard]] const RenderLayerDescriptor& GetLayer(std::size_t index) const noexcept;

private:
    RenderLayerRegistry();

    void RegisterBuiltInLayers();

    Array<RenderLayerDescriptor> layers_{};
    RenderLayerId defaultLayerId_ = 0;
};

}  // namespace Spark
