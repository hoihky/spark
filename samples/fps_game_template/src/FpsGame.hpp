#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/Game.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/FlyCamera.hpp"
#include "spark/scene/Mesh.hpp"

namespace Spark {

class GameObject;
class TextOverlayComponent;

struct TracerBullet {
    GameObject* go = nullptr;
    Vector3 velocity{};
    float timeLeft = 0.0F;
};

/**
 * Minimal first-person template: fly camera, simple arena, LMB spawns a fast emissive tracer and hitscans targets.
 * Extend this class with weapons, animation, physics, and asset pipelines.
 */
class FpsGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override;
    void OnDetach() override;
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    void OnRender(IRenderFrame& frame, IEngineContext& context) override;

private:
    void MountUiFontIfNeeded(GameWorld& world);
    void SpawnArena(GameWorld& world);
    void TryShootTarget(IEngineContext& context);
    void SpawnTracerBullet(const Vector3& origin, const Vector3& dirUnit);
    void UpdateTracerBullets(float deltaSeconds);
    void DestroyAllTracerBullets();
    void CloseWindowIfRequested(IEngineContext& context) const;

    FlyCamera camera{};
    SharedPtr<Mesh> unitCube{};
    SharedPtr<Mesh> groundMesh{};
    Array<GameObject*> roots{};
    Array<GameObject*> targets{};
    TextOverlayComponent* hudText = nullptr;
    std::uint32_t shotsFired = 0;
    std::uint32_t hits = 0;
    float sceneTimeSeconds = 0.0F;
    Array<TracerBullet> tracers{};
};

}  // namespace Spark
