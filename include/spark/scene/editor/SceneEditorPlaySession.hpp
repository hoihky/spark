#pragma once

#include "spark/scene/camera/FlyCamera.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SpawnPointService.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;

/**
 * State object for scene-editor play mode (State pattern).
 * Bookmarks the edit camera, reloads the level, and focuses the Player spawn.
 */
class SceneEditorPlaySession final {
public:
    struct CameraBookmark {
        FlyCamera camera{};
        Vector3 orbitPivot{Vector3::Zero};
        float orbitDistance = 18.0F;
    };

    struct Dependencies {
        SceneLoadSession& loadSession;
        SceneEditorContentModel& content;
        SceneInstanceTracker& instances;
        FlyCamera& camera;
        Vector3& orbitPivot;
        float& orbitDistance;
        void (*reloadScene)(GameWorld& world, void* userData) noexcept = nullptr;
        void (*setStatus)(const char* message, void* userData) noexcept = nullptr;
        void* userData = nullptr;
        const char* spawnName = "Player";
    };

    [[nodiscard]] bool IsActive() const noexcept { return active; }

    void Enter(GameWorld& world, IEngineContext& context, Dependencies& deps);
    void Exit(GameWorld& world, IEngineContext& context, Dependencies& deps);

private:
    void FocusCameraAtSpawn(GameWorld& world, Dependencies& deps) noexcept;

    bool active = false;
    CameraBookmark editBookmark{};
    SpawnPointService spawnPoints{};
};

}  // namespace Spark
