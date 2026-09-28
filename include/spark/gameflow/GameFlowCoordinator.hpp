#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/gameplay/GameStateComponent.hpp"
#include "spark/gameflow/GameFlowPersistentData.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneManager.hpp"

namespace Spark {

/**
 * Application-level flow (title → play → pause → victory → reload) without owning ECS entities.
 * Pair with a scene-local <c>GameStateComponent</c> and optional <c>SceneManager</c> for level reload.
 */
class GameFlowCoordinator final {
public:
    void Bind(GameStateComponent* stateComponent, GameFlowPersistentData* persistentData) noexcept;

    void SetActiveLevelScenePath(const char* path) noexcept;
    [[nodiscard]] const char* GetActiveLevelScenePath() const noexcept { return activeLevelScenePath.CStr(); }

    void RequestIntro() noexcept;
    void RequestPlaying() noexcept;
    void TogglePause() noexcept;
    void RequestVictory() noexcept;
    void RequestDefeat() noexcept;

    /** Unloads all scenes and reloads <c>activeLevelScenePath</c> when set. */
    [[nodiscard]] SceneInstanceId ReloadActiveLevel(SceneManager& scenes, const SceneLoadOptions& options = {});

    void LoadProgressFromDisk(const char* savePath) noexcept;
    void SaveProgressToDisk(const char* savePath) const noexcept;

    void SyncProgressFromGameplay(int gemsCollected, int gemsTotal, int enemiesDefeated, bool levelComplete) noexcept;

private:
    GameStateComponent* gameState = nullptr;
    GameFlowPersistentData* persistent = nullptr;
    Utf8String activeLevelScenePath{};
    SceneInstanceId activeInstanceId = kInvalidSceneInstanceId;
};

}  // namespace Spark
