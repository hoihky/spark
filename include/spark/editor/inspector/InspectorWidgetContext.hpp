#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/editor/EditorTypes.hpp"

namespace Spark {

class GameObject;
class GameWorld;

namespace Editor {

class EditorTextureCatalog;

/** Shared state passed to inspector widgets each frame. */
struct InspectorWidgetContext {
    GameObject* target = nullptr;
    GameWorld* world = nullptr;
    EditorCommandStack* commandStack = nullptr;
    const EditorTextureCatalog* textureCatalog = nullptr;
    EditorMode mode = EditorMode::Edit;
    bool externalTransformEditActive = false;
    GameObject* externalTransformTarget = nullptr;

    [[nodiscard]] bool CanEdit() const noexcept {
        return CanApplyLiveEdit() && commandStack != nullptr && world != nullptr;
    }

    [[nodiscard]] bool CanApplyLiveEdit() const noexcept {
        return mode == EditorMode::Edit && target != nullptr;
    }
};

}  // namespace Editor
}  // namespace Spark
