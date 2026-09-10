#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class MaterialComponent;
class Mesh;
class MeshComponent;
class MultiMaterialComponent;
class Scene;
class WaterBodyComponent;

/** Collects visible water bodies into <c>SceneRenderParams::waterDraws</c>. */
class SceneWaterSubmit final {
public:
    void SubmitFromWorld(
            GameWorld& world,
            const Matrix4& viewProjection,
            const Vector3& cameraPositionWorld,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
            const Scene* sceneForCulling = nullptr);

private:
    [[nodiscard]] static float SquaredDistanceFromCamera(
            const Matrix4& world,
            const Vector3& cameraPositionWorld) noexcept;

    void SortDrawsBackToFront(Array<SceneWaterDraw>& items, const Vector3& cameraPositionWorld) const;

    void PushMeshDraws(
            Array<SceneWaterDraw>& outDraws,
            SceneDrawItem baseItem,
            const Mesh& mesh,
            const MaterialComponent* mat,
            const MultiMaterialComponent* multiMat,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
            float sortDepth,
            float waveTimeSeconds,
            const WaterWaveSettings& waveSettings) const;

    void AppendBodyDraw(
            GameObject& owner,
            const WaterBodyComponent& water,
            const MeshComponent& mesh,
            const MaterialComponent* mat,
            const Matrix4& worldM,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture,
            const Vector3& cameraPositionWorld) const;
};

}  // namespace Spark
