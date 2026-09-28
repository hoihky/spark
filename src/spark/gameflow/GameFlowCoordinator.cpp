#include "spark/gameflow/GameFlowCoordinator.hpp"

#include "spark/save/GameSave.hpp"
#include "spark/scene/core/SceneManager.hpp"

namespace Spark {

void GameFlowCoordinator::Bind(GameStateComponent* stateComponent, GameFlowPersistentData* persistentData) noexcept {
    gameState = stateComponent;
    persistent = persistentData;
}

void GameFlowCoordinator::SetActiveLevelScenePath(const char* path) noexcept {
    activeLevelScenePath = path != nullptr ? Utf8String(path) : Utf8String{};
}

void GameFlowCoordinator::RequestIntro() noexcept {
    if (gameState != nullptr) {
        gameState->RequestState(GameFlowState::Intro);
    }
}

void GameFlowCoordinator::RequestPlaying() noexcept {
    if (gameState != nullptr) {
        gameState->RequestState(GameFlowState::Playing);
    }
}

void GameFlowCoordinator::TogglePause() noexcept {
    if (gameState == nullptr) {
        return;
    }
    if (gameState->IsState(GameFlowState::Playing)) {
        gameState->PushState(GameFlowState::Paused);
        return;
    }
    if (gameState->IsState(GameFlowState::Paused)) {
        gameState->PopState();
    }
}

void GameFlowCoordinator::RequestVictory() noexcept {
    if (gameState != nullptr) {
        gameState->RequestState(GameFlowState::Victory);
    }
    if (persistent != nullptr) {
        persistent->progress.levelComplete = true;
    }
}

void GameFlowCoordinator::RequestDefeat() noexcept {
    if (gameState != nullptr) {
        gameState->RequestState(GameFlowState::Defeat);
    }
}

SceneInstanceId GameFlowCoordinator::ReloadActiveLevel(SceneManager& scenes, const SceneLoadOptions& options) {
    if (activeLevelScenePath.IsEmpty()) {
        return kInvalidSceneInstanceId;
    }
    if (activeInstanceId != kInvalidSceneInstanceId) {
        scenes.UnloadScene(activeInstanceId);
        activeInstanceId = kInvalidSceneInstanceId;
    }
    activeInstanceId = scenes.LoadSceneFromFile(activeLevelScenePath.CStr(), options);
    if (gameState != nullptr) {
        gameState->RequestState(GameFlowState::Playing);
    }
    if (persistent != nullptr) {
        persistent->progress.levelComplete = false;
    }
    return activeInstanceId;
}

void GameFlowCoordinator::LoadProgressFromDisk(const char* savePath) noexcept {
    if (persistent == nullptr || savePath == nullptr) {
        return;
    }
    GameSaveSlot slot{};
    if (GameSave::TryLoad(savePath, slot)) {
        persistent->ApplyFromSlot(slot);
    }
}

void GameFlowCoordinator::SaveProgressToDisk(const char* savePath) const noexcept {
    if (persistent == nullptr || savePath == nullptr) {
        return;
    }
    GameSaveSlot slot{};
    persistent->WriteToSlot(slot);
    (void)GameSave::TrySave(savePath, slot);
}

void GameFlowCoordinator::SyncProgressFromGameplay(
        const int gemsCollectedIn,
        const int gemsTotalIn,
        const int enemiesDefeatedIn,
        const bool levelCompleteIn) noexcept {
    if (persistent == nullptr) {
        return;
    }
    persistent->progress.gemsCollected = gemsCollectedIn;
    persistent->progress.gemsTotal = gemsTotalIn;
    persistent->progress.enemiesDefeated = enemiesDefeatedIn;
    persistent->progress.levelComplete = levelCompleteIn;
}

}  // namespace Spark
