#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/scene/ScenePartitionKind.hpp"

namespace Spark {

class GameWorld;
class GameObject;
class MeshComponent;
class MaterialComponent;
class SkinnedMeshComponent;
class AnimatorComponent;
class Scene;

/** Callback for rigid mesh draws that pass the active frustum / spatial query. */
class DrawableFrustumSink {
public:
    virtual ~DrawableFrustumSink() = default;
    virtual void OnDrawable(
            GameObject* object,
            const MeshComponent& mesh,
            const MaterialComponent* material,
            const Matrix4& worldMatrix) = 0;
};

/** Callback for skinned draws after the same frustum pipeline. */
class SkinnedDrawableFrustumSink {
public:
    virtual ~SkinnedDrawableFrustumSink() = default;
    virtual void OnSkinnedDrawable(GameObject* object,
            const SkinnedMeshComponent& skinned,
            const MaterialComponent* material,
            const AnimatorComponent* animator,
            const Matrix4& worldMatrix) = 0;
};

void DispatchDrawableFrustumCull(
        const Scene& scene, const Matrix4& viewProjection, ScenePartitionKind mode, DrawableFrustumSink& sink);

void DispatchSkinnedDrawableFrustumCull(
        const GameWorld& world,
        const Matrix4& viewProjection,
        ScenePartitionKind mode,
        SkinnedDrawableFrustumSink& sink);

void DispatchSkinnedDrawableFrustumCull(const Scene& scene,
        const Matrix4& viewProjection,
        ScenePartitionKind mode,
        SkinnedDrawableFrustumSink& sink);

}  // namespace Spark
