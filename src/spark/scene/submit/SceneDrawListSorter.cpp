#include "spark/scene/submit/SceneDrawListSorter.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/submit/DrawableSortKey.hpp"
#include "spark/scene/submit/DrawableSortResolver.hpp"

namespace Spark {

namespace {

int DrawSortLayer(SceneMeshSlot slot) {
    switch (slot) {
    case SceneMeshSlot::GroundPlane:
        return 0;
    case SceneMeshSlot::UnitCube:
        return 1;
    case SceneMeshSlot::Custom:
        return 2;
    }
    return 1;
}

[[nodiscard]] bool IsTransparentSceneDraw(const SceneDrawItem& item) noexcept {
    return item.skyMode == SceneSkyMode::None &&
           (item.opacity < 0.999F || item.alphaBlend || item.gltfExtensions.transmissionFactor > 0.01F);
}

void SortTransparentBackToFront(
        Array<SceneDrawItem>& items,
        const Vector3& cameraPositionWorld,
        SceneTransparentSortMode mode) {
    if (mode != SceneTransparentSortMode::BackToFrontByDepth) {
        return;
    }
    const auto fartherFirst = [&cameraPositionWorld](const SceneDrawItem& a, const SceneDrawItem& b) noexcept -> bool {
        const float ax = a.model.m[12] - cameraPositionWorld.x;
        const float ay = a.model.m[13] - cameraPositionWorld.y;
        const float az = a.model.m[14] - cameraPositionWorld.z;
        const float bx = b.model.m[12] - cameraPositionWorld.x;
        const float by = b.model.m[13] - cameraPositionWorld.y;
        const float bz = b.model.m[14] - cameraPositionWorld.z;
        return (ax * ax + ay * ay + az * az) > (bx * bx + by * by + bz * bz);
    };
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneDrawItem key = items[i];
        std::size_t j = i;
        while (j > 0 && fartherFirst(items[j - 1], key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

[[nodiscard]] float SquaredDistanceFromCamera(const Matrix4& world, const Vector3& cameraPositionWorld) noexcept {
    const float dx = world.m[12] - cameraPositionWorld.x;
    const float dy = world.m[13] - cameraPositionWorld.y;
    const float dz = world.m[14] - cameraPositionWorld.z;
    return dx * dx + dy * dy + dz * dz;
}

}  // namespace

int SceneDrawListSorter::OpaqueDrawSortKey(const SceneDrawItem& item) noexcept {
    if (item.skyMode != SceneSkyMode::None) {
        return -100000;
    }
    const int queueBand = static_cast<int>(item.renderQueue) * 1'000'000;
    const int layerBand = static_cast<int>(item.drawableSort.sortingLayerOrder) * 1'000;
    const int orderInLayer = item.drawableSort.sortingOrder;
    const int meshBand = DrawSortLayer(item.mesh);
    return queueBand + layerBand + orderInLayer + meshBand;
}

void SceneDrawListSorter::StableSortOpaqueDraws(Array<SceneDrawItem>& items) noexcept {
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneDrawItem key = items[i];
        std::size_t j = i;
        while (j > 0 && OpaqueDrawSortKey(items[j - 1]) > OpaqueDrawSortKey(key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

SceneBlendMode SceneDrawListSorter::ResolveSpriteBlendMode(const GameObject& object) noexcept {
    if (const BlendModeComponent* blend = object.GetComponent<BlendModeComponent>()) {
        return blend->GetMode();
    }
    return kSceneBlendModeDefault;
}

void SceneDrawListSorter::StableSortSprites(Array<SceneSpriteDraw>& items, const SceneSpriteSortMode mode) noexcept {
    const auto moreInFront = [mode](const SceneSpriteDraw& a, const SceneSpriteDraw& b) noexcept -> bool {
        const std::uint8_t blendA = GetSceneBlendModePassOrder(a.blendMode);
        const std::uint8_t blendB = GetSceneBlendModePassOrder(b.blendMode);
        if (blendA != blendB) {
            return blendA > blendB;
        }
        const DrawableSortKey keyA{a.sortingLayerOrder, a.sortOrder};
        const DrawableSortKey keyB{b.sortingLayerOrder, b.sortOrder};
        if (DrawableSortMoreInFront(keyA, keyB)) {
            return true;
        }
        if (DrawableSortMoreInFront(keyB, keyA)) {
            return false;
        }
        if (mode == SceneSpriteSortMode::SortOrderThenWorldY) {
            return a.sortWorldY < b.sortWorldY;
        }
        return false;
    };
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneSpriteDraw key = items[i];
        std::size_t j = i;
        while (j > 0 && moreInFront(items[j - 1], key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

void SceneDrawListSorter::StableSortTilemaps(Array<SceneTilemapDraw>& draws) noexcept {
    const auto moreInFront = [](const SceneTilemapDraw& a, const SceneTilemapDraw& b) noexcept -> bool {
        const DrawableSortKey keyA{a.sortingLayerOrder, a.sortOrderBase};
        const DrawableSortKey keyB{b.sortingLayerOrder, b.sortOrderBase};
        if (DrawableSortMoreInFront(keyA, keyB)) {
            return true;
        }
        if (DrawableSortMoreInFront(keyB, keyA)) {
            return false;
        }
        return a.sortWorldYAnchor < b.sortWorldYAnchor;
    };
    const std::size_t n = draws.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneTilemapDraw key = draws[i];
        std::size_t j = i;
        while (j > 0 && moreInFront(draws[j - 1], key)) {
            draws[j] = draws[j - 1];
            --j;
        }
        draws[j] = key;
    }
}

void SceneDrawListSorter::StableSortWaterBackToFront(
        Array<SceneWaterDraw>& items,
        const Vector3& cameraPositionWorld,
        const SceneWaterSortMode sortMode) noexcept {
    if (sortMode != SceneWaterSortMode::BackToFrontByDepth) {
        return;
    }
    const auto fartherFirst = [&cameraPositionWorld](const SceneWaterDraw& a, const SceneWaterDraw& b) noexcept -> bool {
        if (a.sortDepth != b.sortDepth) {
            return a.sortDepth > b.sortDepth;
        }
        return SquaredDistanceFromCamera(a.item.model, cameraPositionWorld) >
               SquaredDistanceFromCamera(b.item.model, cameraPositionWorld);
    };
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneWaterDraw key = items[i];
        std::size_t j = i;
        while (j > 0 && fartherFirst(items[j - 1], key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

void SceneDrawListSorter::PartitionIntoSceneParams(
        const Array<SceneDrawItem>& sortedDrawList,
        SceneRenderParams& params,
        const Vector3& cameraPositionWorld) noexcept {
    for (std::size_t di = 0; di < sortedDrawList.GetSize(); ++di) {
        SceneDrawItem item = sortedDrawList[di];
        if (IsTransparentSceneDraw(item)) {
            item.renderQueue = SceneRenderQueue::Transparent;
            params.transparentDraws.PushBack(item);
        } else {
            params.draws.PushBack(item);
        }
    }
    SortTransparentBackToFront(params.transparentDraws, cameraPositionWorld, params.transparentSortMode);
}

namespace SceneSubmitDetail {

void StableSortDrawItems(Array<SceneDrawItem>& items) {
    SceneDrawListSorter::StableSortOpaqueDraws(items);
}

void StableSortSprites(Array<SceneSpriteDraw>& items, const SceneSpriteSortMode mode) {
    SceneDrawListSorter::StableSortSprites(items, mode);
}

SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept {
    return SceneDrawListSorter::ResolveSpriteBlendMode(object);
}

}  // namespace SceneSubmitDetail

SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept {
    return SceneSubmitDetail::ResolveSpriteBlendMode(object);
}

void PartitionSortedDrawItemsIntoSceneParams(
        const Array<SceneDrawItem>& sortedDrawList,
        SceneRenderParams& params,
        const Vector3& cameraPositionWorld) noexcept {
    SceneDrawListSorter::PartitionIntoSceneParams(sortedDrawList, params, cameraPositionWorld);
}

}  // namespace Spark