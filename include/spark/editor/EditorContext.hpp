#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorProject.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/EditorTypes.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"

namespace Spark {

class GameWorld;
class IEngineContext;
class Scene;

class ISceneEditorAssetBrowserHost;
class SceneEditorContentModel;

namespace Editor {

class EditorCommandStack;
class EditorTextureCatalog;
class EditorViewport;

struct EditorContext {
    GameWorld* world = nullptr;
    Scene* scene = nullptr;
    IEngineContext* engine = nullptr;
    EditorSelection* selection = nullptr;
    EditorProject* project = nullptr;
    EditorCommandStack* commandStack = nullptr;
    EditorViewport* viewport = nullptr;
    EditorTextureCatalog* textureCatalog = nullptr;
    SceneEditorContentModel* contentModel = nullptr;
    SceneEditorAssetCatalog* assetCatalog = nullptr;
    ISceneEditorAssetBrowserHost* assetHost = nullptr;
    EditorMode mode = EditorMode::Edit;
    WorkspaceDimension workspace = WorkspaceDimension::ThreeD;
    Utf8String statusLine;
};

}  // namespace Editor
}  // namespace Spark
