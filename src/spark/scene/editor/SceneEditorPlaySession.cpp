#include "spark/scene/editor/SceneEditorPlaySession.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark {

void SceneEditorPlaySession::FocusCameraAtSpawn(GameWorld& world, Dependencies& deps) noexcept {
    const SceneSpawnPose spawn = spawnPoints.Resolve(world, deps.spawnName);
    if (!spawn.found) {
        return;
    }
    deps.orbitPivot = spawn.position;
    deps.camera.position = spawn.position + Vector3{0.0F, 2.0F, 6.0F};
    deps.camera.SnapLookAt(spawn.position);
    const Vector3 offset{
            deps.camera.position.x - deps.orbitPivot.x,
            deps.camera.position.y - deps.orbitPivot.y,
            deps.camera.position.z - deps.orbitPivot.z};
    deps.orbitDistance = std::max(3.0F, offset.Length());
}

void SceneEditorPlaySession::Enter(GameWorld& world, IEngineContext& context, Dependencies& deps) {
    if (active) {
        return;
    }

    editBookmark.camera = deps.camera;
    editBookmark.orbitPivot = deps.orbitPivot;
    editBookmark.orbitDistance = deps.orbitDistance;

    if (deps.reloadScene != nullptr) {
        deps.reloadScene(world, deps.userData);
    }

    FocusCameraAtSpawn(world, deps);
    context.GetInput().SetCursorCaptured(true);
    active = true;

    if (deps.setStatus != nullptr) {
        deps.setStatus("Play mode — Esc to return to editor.", deps.userData);
    }
}

void SceneEditorPlaySession::Exit(GameWorld& world, IEngineContext& context, Dependencies& deps) {
    if (!active) {
        return;
    }
    active = false;
    context.GetInput().SetCursorCaptured(false);

    if (deps.reloadScene != nullptr) {
        deps.reloadScene(world, deps.userData);
    }

    deps.camera = editBookmark.camera;
    deps.orbitPivot = editBookmark.orbitPivot;
    deps.orbitDistance = editBookmark.orbitDistance;

    if (deps.setStatus != nullptr) {
        deps.setStatus("Returned to editor.", deps.userData);
    }
}

}  // namespace Spark
