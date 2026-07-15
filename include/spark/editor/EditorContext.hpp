#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorProject.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/EditorTypes.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class Scene;

namespace Editor {

/** Non-owning bundle passed to editor panels each frame. */
struct EditorContext {
    GameWorld* world = nullptr;
    Scene* scene = nullptr;
    IEngineContext* engine = nullptr;
    EditorSelection* selection = nullptr;
    EditorProject* project = nullptr;
    EditorMode mode = EditorMode::Edit;
    WorkspaceDimension workspace = WorkspaceDimension::ThreeD;
    Utf8String statusLine;
};

}  // namespace Editor
}  // namespace Spark
