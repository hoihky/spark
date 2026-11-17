#pragma once

#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/lighting/SceneLightingSetup.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class MaterialComponent;
class Scene;
struct SceneDrawItem;

/**
 * Public API for lit ECS → <c>SceneRenderParams</c>: static helpers plus fluent <c>Builder</c>.
 * Long free functions in <c>SceneSubmit.hpp</c> remain for compatibility.
 */
class LitScene final {
public:
    class Builder {
    public:
        Builder(GameWorld& world, IEngineContext& context) noexcept;

        Builder& WithView(const Matrix4& viewProjection, const Vector3& cameraPositionWorld) noexcept;
        [[nodiscard]] bool WithCameraFromWorld() noexcept;

        Builder& WithOptions(const LitSceneSubmitOptions& options) noexcept;
        Builder& WithLightingSetup(const SceneLightingSetup& setup) noexcept;

        [[nodiscard]] bool Fill(SceneRenderParams& outParams) const noexcept;
        [[nodiscard]] bool Submit() const noexcept;

    private:
        GameWorld& hostWorld;
        IEngineContext& engineContext;
        LitSceneSubmitOptions submitOptions{};
        SceneLightingSetup lightingSetup{SceneLightingSetup::FromProfile(SceneLightingProfile::Default)};
        bool hasLightingSetup = false;
        Matrix4 viewProjection{};
        Vector3 cameraPositionWorld{};
        bool hasView = false;
    };

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
        SceneDrawListSorter::PartitionIntoSceneParams(sortedDrawList, params, cameraPositionWorld);
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