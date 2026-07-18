#include "spark/scene/SceneSubmit.hpp"
#include "spark/scene/detail/SceneSubmitDetail.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/DrawableSortKey.hpp"
#include "spark/scene/DrawableSortResolver.hpp"

namespace Spark {

namespace {

int DrawSortLayer(SceneMeshSlot s) {
    switch (s) {
    case SceneMeshSlot::GroundPlane:
        return 0;
    case SceneMeshSlot::UnitCube:
        return 1;
    case SceneMeshSlot::Custom:
        return 2;
    }
    return 1;
}

int DrawSortKey(const SceneDrawItem& it) {
    if (it.skyMode != SceneSkyMode::None) {
        return -100;
    }
    return DrawSortLayer(it.mesh);
}

[[nodiscard]] bool IsTransparentSceneDraw(const SceneDrawItem& item) noexcept {
    return item.skyMode == SceneSkyMode::None && item.opacity < 0.999F;
}

void SortTransparentDrawsBackToFront(
        Array<SceneDrawItem>& items,
        const Vector3& cameraPositionWorld,
        const SceneTransparentSortMode mode) {
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

}  // namespace

namespace SceneSubmitDetail {

void StableSortDrawItems(Array<SceneDrawItem>& items) {
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneDrawItem key = items[i];
        std::size_t j = i;
        while (j > 0 && DrawSortKey(items[j - 1]) > DrawSortKey(key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept {
    if (const BlendModeComponent* blend = object.GetComponent<BlendModeComponent>()) {
        return blend->GetMode();
    }
    return kSceneBlendModeDefault;
}

void StableSortSprites(Array<SceneSpriteDraw>& items, SceneSpriteSortMode mode) {
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

}  // namespace SceneSubmitDetail

void PartitionSortedDrawItemsIntoSceneParams(
        const Array<SceneDrawItem>& sortedDrawList,
        SceneRenderParams& params,
        const Vector3& cameraPositionWorld) noexcept {
    for (std::size_t di = 0; di < sortedDrawList.GetSize(); ++di) {
        if (IsTransparentSceneDraw(sortedDrawList[di])) {
            params.transparentDraws.PushBack(sortedDrawList[di]);
        } else {
            params.draws.PushBack(sortedDrawList[di]);
        }
    }
    SortTransparentDrawsBackToFront(params.transparentDraws, cameraPositionWorld, params.transparentSortMode);
}

}  // namespace Spark
