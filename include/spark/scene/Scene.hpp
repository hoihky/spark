#pragma once

#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/CameraComponent.hpp"
#include "spark/ecs/components/lighting/DirectionalLightComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/SceneDrawableFrustumSink.hpp"
#include "spark/scene/SceneRaycast.hpp"

namespace Spark {

/**
 * Scene container: owns the GameWorld (ECS + assets) and helpers to traverse drawable objects.
 */
class Scene {
public:
    [[nodiscard]] GameWorld& GetWorld() noexcept { return world; }
    [[nodiscard]] const GameWorld& GetWorld() const noexcept { return world; }

    /** Frustum / spatial traversal mode for ForEach*InViewFrustum (default None = legacy full scene visit). */
    void SetSpatialPartitionKind(ScenePartitionKind k) noexcept { sceneSpatialPartitionKind = k; }
    [[nodiscard]] ScenePartitionKind GetSpatialPartitionKind() const noexcept { return sceneSpatialPartitionKind; }

    /**
     * Copies partition kind from the first GameObject that has a SceneSpatialPolicyComponent (if any).
     * Call from your frame loop when designers drive culling mode from ECS.
     */
    void ApplySpatialPolicyFromFirstMatchingObject() noexcept {
        bool found = false;
        world.ForEachGameObject([this, &found](GameObject* o) {
            if (found || o == nullptr) {
                return;
            }
            if (const SceneSpatialPolicyComponent* p = o->GetComponent<SceneSpatialPolicyComponent>()) {
                sceneSpatialPartitionKind = p->GetPartitionKind();
                found = true;
            }
        });
    }

    /**
     * Same callback shape as ForEachDrawable, but only invokes objects whose world mesh AABB intersects
     * the view frustum from `viewProjection` (proj * view, column-major). Uses GetSpatialPartitionKind().
     */
    template<typename Fn>
    void ForEachDrawableInViewFrustum(const Matrix4& viewProjection, Fn&& fn) const {
        if (sceneSpatialPartitionKind == ScenePartitionKind::None) {
            ForEachDrawable(std::forward<Fn>(fn));
            return;
        }
        struct LocalSink final : DrawableFrustumSink {
            Fn& f;
            explicit LocalSink(Fn& inF) noexcept : f(inF) {}
            void OnDrawable(GameObject* object,
                    const MeshComponent& mesh,
                    const MaterialComponent* material,
                    const Matrix4& worldMatrix) override {
                f(object, mesh, material, worldMatrix);
            }
        } sink{fn};
        DispatchDrawableFrustumCull(*this, viewProjection, sceneSpatialPartitionKind, sink);
    }

    template<typename Fn>
    void ForEachSkinnedDrawableInViewFrustum(const Matrix4& viewProjection, Fn&& fn) const {
        if (sceneSpatialPartitionKind == ScenePartitionKind::None) {
            ForEachSkinnedDrawable(std::forward<Fn>(fn));
            return;
        }
        struct LocalSink final : SkinnedDrawableFrustumSink {
            Fn& f;
            explicit LocalSink(Fn& inF) noexcept : f(inF) {}
            void OnSkinnedDrawable(GameObject* object,
                    const SkinnedMeshComponent& skinned,
                    const MaterialComponent* material,
                    const AnimatorComponent* animator,
                    const Matrix4& worldMatrix) override {
                f(object, skinned, material, animator, worldMatrix);
            }
        } sink{fn};
        DispatchSkinnedDrawableFrustumCull(*this, viewProjection, sceneSpatialPartitionKind, sink);
    }

    /**
     * Invokes fn(gameObject, meshComponent, materialOrNull, worldMatrix) for every object that has a
     * MeshComponent with a non-null mesh. MaterialComponent is optional (textures / tint).
     */
    template<typename Fn>
    void ForEachDrawable(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const MeshComponent* mc = o->GetComponent<MeshComponent>();
            if (mc == nullptr || !mc->GetMesh()) {
                return;
            }
            const MaterialComponent* mat = o->GetComponent<MaterialComponent>();
            const Matrix4 worldMatrix = o->GetWorldMatrix();
            fn(o, *mc, mat, worldMatrix);
        });
    }

    /**
     * Skinned draws: SkinnedMeshComponent + optional MaterialComponent; animator supplies skeleton sampling.
     * Caller should sample AnimatorComponent + skeleton into joint palette when building SceneDrawItem.
     */
    template<typename Fn>
    void ForEachSkinnedDrawable(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const SkinnedMeshComponent* smc = o->GetComponent<SkinnedMeshComponent>();
            if (smc == nullptr || !smc->GetMesh()) {
                return;
            }
            const MaterialComponent* mat = o->GetComponent<MaterialComponent>();
            const AnimatorComponent* anim = o->GetComponent<AnimatorComponent>();
            const Matrix4 worldMatrix = o->GetWorldMatrix();
            fn(o, *smc, mat, anim, worldMatrix);
        });
    }

    /**
     * Invokes fn(pointLightComponent, worldMatrix) for every object with an enabled PointLightComponent.
     * World translation is taken from worldMatrix (column 3); use when building SceneRenderParams::pointLights.
     */
    template<typename Fn>
    void ForEachPointLight(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const PointLightComponent* pl = o->GetComponent<PointLightComponent>();
            if (pl == nullptr || !pl->IsEnabled()) {
                return;
            }
            fn(*pl, o->GetWorldMatrix());
        });
    }

    /** Invokes fn(spotLightComponent, worldMatrix) for every object with an enabled SpotLightComponent. */
    template<typename Fn>
    void ForEachSpotLight(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const SpotLightComponent* sl = o->GetComponent<SpotLightComponent>();
            if (sl == nullptr || !sl->IsEnabled()) {
                return;
            }
            fn(*sl, o->GetWorldMatrix());
        });
    }

    /** Invokes fn(directionalLightComponent, worldMatrix) for every enabled DirectionalLightComponent. */
    template<typename Fn>
    void ForEachDirectionalLight(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const DirectionalLightComponent* dl = o->GetComponent<DirectionalLightComponent>();
            if (dl == nullptr || !dl->IsEnabled()) {
                return;
            }
            fn(*dl, o->GetWorldMatrix());
        });
    }

    /** Invokes fn(cameraComponent, worldMatrix) for every object with an enabled CameraComponent. */
    template<typename Fn>
    void ForEachCamera(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const CameraComponent* cam = o->GetComponent<CameraComponent>();
            if (cam == nullptr || !cam->IsEnabled()) {
                return;
            }
            fn(*cam, o->GetWorldMatrix());
        });
    }

    /** Invokes fn(camera2dComponent, worldMatrix) for every object with an enabled Camera2DComponent. */
    template<typename Fn>
    void ForEachCamera2D(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const Camera2DComponent* cam = o->GetComponent<Camera2DComponent>();
            if (cam == nullptr || !cam->IsEnabled()) {
                return;
            }
            fn(*cam, o->GetWorldMatrix());
        });
    }

    /** Screen-space text overlays (TextOverlayComponent). */
    template<typename Fn>
    void ForEachTextOverlay(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const TextOverlayComponent* tc = o->GetComponent<TextOverlayComponent>();
            if (tc == nullptr || !tc->IsVisible()) {
                return;
            }
            fn(*tc);
        });
    }

    /**
     * Sky environment: SkyComponent + MeshComponent + optional MaterialComponent (texture).
     * Typical: one active sky per scene; sort sky before opaque so geometry can overdraw sky without depth ties.
     */
    template<typename Fn>
    void ForEachSky(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const SkyComponent* sky = o->GetComponent<SkyComponent>();
            if (sky == nullptr || !sky->IsSkyEnabled()) {
                return;
            }
            const MeshComponent* mesh = o->GetComponent<MeshComponent>();
            if (mesh == nullptr || !mesh->GetMesh()) {
                return;
            }
            const MaterialComponent* mat = o->GetComponent<MaterialComponent>();
            fn(*o, *sky, *mesh, mat, o->GetWorldMatrix());
        });
    }

    /**
     * ParticleEmitterComponent on enabled objects. Use when building SceneRenderParams::particles
     * (after GameWorld::UpdateGameObjects so simulation is current).
     */
    template<typename Fn>
    void ForEachParticleEmitter(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const ParticleEmitterComponent* pe = o->GetComponent<ParticleEmitterComponent>();
            if (pe == nullptr || !pe->IsEmitterEnabled()) {
                return;
            }
            fn(*pe, o->GetWorldMatrix());
        });
    }

    /** GUI roots (GuiCanvasComponent); use for custom tooling or with spark/gui/GuiScene helpers. */
    template<typename Fn>
    void ForEachGuiCanvas(Fn&& fn) const {
        world.ForEachGameObject([&fn](GameObject* o) {
            if (o == nullptr) {
                return;
            }
            const GuiCanvasComponent* gc = o->GetComponent<GuiCanvasComponent>();
            if (gc == nullptr || !gc->IsCanvasEnabled()) {
                return;
            }
            fn(*gc);
        });
    }

    /**
     * CPU mesh pick against rigid and skinned drawables (bind pose for skinned).
     * Returns the nearest hit along the ray in [tMin, tMax].
     */
    [[nodiscard]] bool RaycastPick(
            const SceneRay& ray,
            SceneRaycastHit& outHit,
            const SceneRaycastOptions& options = {}) const;

private:
    GameWorld world;
    ScenePartitionKind sceneSpatialPartitionKind = ScenePartitionKind::None;
};

}  // namespace Spark
