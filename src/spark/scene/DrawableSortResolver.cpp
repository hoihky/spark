#include "spark/scene/DrawableSortResolver.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/RenderLayerComponent.hpp"
#include "spark/ecs/components/SortingGroupComponent.hpp"
#include "spark/scene/RenderLayerRegistry.hpp"

namespace Spark {

namespace {

[[nodiscard]] const RenderLayerComponent* FindRenderLayerOnObject(const GameObject& object) noexcept {
    return object.GetComponent<RenderLayerComponent>();
}

[[nodiscard]] std::int32_t SaturateSortingOrder(const std::int64_t value) noexcept {
    constexpr std::int64_t kMin = static_cast<std::int64_t>(-2147483647LL);
    constexpr std::int64_t kMax = static_cast<std::int64_t>(2147483647LL);
    if (value < kMin) {
        return static_cast<std::int32_t>(kMin);
    }
    if (value > kMax) {
        return static_cast<std::int32_t>(kMax);
    }
    return static_cast<std::int32_t>(value);
}

}  // namespace

const SortingGroupComponent* DrawableSortResolver::FindNearestSortingGroup(
        const GameObject& drawable) noexcept {
    const GameObject* current = &drawable;
    while (current != nullptr) {
        if (const SortingGroupComponent* group = current->GetComponent<SortingGroupComponent>()) {
            if (group->IsEnabled()) {
                return group;
            }
        }
        current = current->GetParent();
    }
    return nullptr;
}

std::int16_t DrawableSortResolver::ResolveLayerSortingOrder(
        const GameObject& drawable,
        const SortingGroupComponent* sortingGroup) noexcept {
    const RenderLayerRegistry& registry = RenderLayerRegistry::Instance();
    const GameObject* layerAnchor = sortingGroup != nullptr ? sortingGroup->GetOwner() : &drawable;
    if (layerAnchor == nullptr) {
        return registry.GetDefaultLayerSortingOrder();
    }
    if (const RenderLayerComponent* layer = FindRenderLayerOnObject(*layerAnchor)) {
        return registry.GetLayerSortingOrder(layer->GetLayerId());
    }
    if (const RenderLayerComponent* drawableLayer = FindRenderLayerOnObject(drawable)) {
        return registry.GetLayerSortingOrder(drawableLayer->GetLayerId());
    }
    return registry.GetDefaultLayerSortingOrder();
}

std::int32_t DrawableSortResolver::ResolveLocalOrderInLayer(
        const GameObject& drawable,
        const std::int32_t nativeOrder) noexcept {
    if (const RenderLayerComponent* layer = FindRenderLayerOnObject(drawable)) {
        if (layer->HasExplicitOrderInLayer()) {
            return layer->GetOrderInLayer();
        }
    }
    return nativeOrder;
}

float DrawableSortResolver::ResolveWorldYAnchor(
        const GameObject& drawable,
        const SortingGroupComponent* sortingGroup) noexcept {
    if (sortingGroup != nullptr && sortingGroup->UsesRootWorldY() && sortingGroup->GetOwner() != nullptr) {
        return sortingGroup->GetOwner()->GetWorldMatrix().m[13];
    }
    return drawable.GetWorldMatrix().m[13];
}

ResolvedDrawableSort DrawableSortResolver::Resolve(const GameObject& drawable, const std::int32_t nativeOrder) noexcept {
    ResolvedDrawableSort resolved{};
    const SortingGroupComponent* sortingGroup = FindNearestSortingGroup(drawable);
    resolved.key.sortingLayerOrder = ResolveLayerSortingOrder(drawable, sortingGroup);
    const std::int32_t localOrder = ResolveLocalOrderInLayer(drawable, nativeOrder);
    if (sortingGroup != nullptr) {
        const std::int64_t combined =
                static_cast<std::int64_t>(sortingGroup->GetSortingOrder()) + static_cast<std::int64_t>(localOrder);
        resolved.key.sortingOrder = SaturateSortingOrder(combined);
    } else {
        resolved.key.sortingOrder = localOrder;
    }
    resolved.worldYAnchor = ResolveWorldYAnchor(drawable, sortingGroup);
    return resolved;
}

}  // namespace Spark
