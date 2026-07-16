#include "spark/scene/RenderLayerRegistry.hpp"

#include <cstring>

namespace Spark {

RenderLayerRegistry& RenderLayerRegistry::Instance() noexcept {
    static RenderLayerRegistry registry;
    return registry;
}

RenderLayerRegistry::RenderLayerRegistry() {
    RegisterBuiltInLayers();
}

void RenderLayerRegistry::RegisterBuiltInLayers() {
    (void)RegisterLayer("Background", 0);
    defaultLayerId = RegisterLayer("Default", 100);
    (void)RegisterLayer("Characters", 200);
    (void)RegisterLayer("Effects", 300);
    (void)RegisterLayer("Overlay", 400);
}

RenderLayerId RenderLayerRegistry::RegisterLayer(const char* name, const std::int16_t sortingOrder) {
    if (name == nullptr || name[0] == '\0') {
        return kInvalidRenderLayerId;
    }
    const RenderLayerId existing = FindLayerIdByName(name);
    if (existing != kInvalidRenderLayerId) {
        return existing;
    }
    if (layers.GetSize() >= static_cast<std::size_t>(kInvalidRenderLayerId)) {
        return kInvalidRenderLayerId;
    }
    RenderLayerDescriptor entry{};
    entry.name = Utf8String(name);
    entry.sortingOrder = sortingOrder;
    layers.PushBack(entry);
    if (std::strcmp(name, "Default") == 0) {
        defaultLayerId = static_cast<RenderLayerId>(layers.GetSize() - 1U);
    }
    return static_cast<RenderLayerId>(layers.GetSize() - 1U);
}

RenderLayerId RenderLayerRegistry::FindLayerIdByName(const char* name) const noexcept {
    if (name == nullptr) {
        return kInvalidRenderLayerId;
    }
    for (std::size_t i = 0; i < layers.GetSize(); ++i) {
        if (std::strcmp(layers[i].name.CStr(), name) == 0) {
            return static_cast<RenderLayerId>(i);
        }
    }
    return kInvalidRenderLayerId;
}

std::int16_t RenderLayerRegistry::GetLayerSortingOrder(const RenderLayerId id) const noexcept {
    if (id >= layers.GetSize()) {
        return GetDefaultLayerSortingOrder();
    }
    return layers[id].sortingOrder;
}

const char* RenderLayerRegistry::GetLayerName(const RenderLayerId id) const noexcept {
    if (id >= layers.GetSize()) {
        return "Default";
    }
    return layers[id].name.CStr();
}

std::int16_t RenderLayerRegistry::GetDefaultLayerSortingOrder() const noexcept {
    return GetLayerSortingOrder(defaultLayerId);
}

const RenderLayerDescriptor& RenderLayerRegistry::GetLayer(const std::size_t index) const noexcept {
    return layers[index];
}

}  // namespace Spark
