#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class MaterialComponent;
class Scene;
struct SceneDrawItem;

/**
 * Shorter names for the standard lit ECS → <c>SceneRenderParams</c> path.
 * Long free functions in <c>SceneSubmit.hpp</c> remain for compatibility.
 */
class LitScene final {
public:
    static void ApplyMaterial(
            SceneDrawItem& item,
            const MaterialComponent* material,
            SceneRenderParams* resolveTextures = nullptr) noexcept {
        ApplyMaterialComponentToSceneDrawItem(item, material, resolveTextures);
    }

    static void ApplyMaterialSlot(
            SceneDrawItem& item,
            const MultiMaterialComponent::Slot& slot,
            SceneRenderParams* resolveTextures = nullptr) noexcept {
        ApplyMultiMaterialSlotToSceneDrawItem(item, slot, resolveTextures);
    }

    static void PartitionDraws(
            const Array<SceneDrawItem>& sortedDrawList,
            SceneRenderParams& params,
            const Vector3& cameraPositionWorld) noexcept {
        PartitionSortedDrawItemsIntoSceneParams(sortedDrawList, params, cameraPositionWorld);
    }

    static void Fill(
            GameWorld& world,
            IEngineContext& context,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const LitSceneSubmitOptions& options,
            SceneRenderParams& outParams) {
        FillStandardLitSceneFromWorld(world, context, viewProjection, cameraPositionWorld, options, outParams);
    }

    static void Submit(
            GameWorld& world,
            IEngineContext& context,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            const LitSceneSubmitOptions& options) {
        SubmitStandardLitSceneFromWorld(world, context, viewProjection, cameraPositionWorld, options);
    }

    [[nodiscard]] static bool TryResolveCamera(
            const GameWorld& world,
            float framebufferWidth,
            float framebufferHeight,
            SceneRenderParams& outParams) noexcept {
        return TryFillSceneCameraFromWorld(world, framebufferWidth, framebufferHeight, outParams);
    }

    [[nodiscard]] static bool SubmitWithCamera(GameWorld& world, IEngineContext& context, const LitSceneSubmitOptions& options) {
        return SubmitStandardLitSceneFromWorldWithCamera(world, context, options);
    }
};

}  // namespace Spark
