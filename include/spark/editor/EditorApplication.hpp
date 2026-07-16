#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorContext.hpp"
#include "spark/editor/EditorProject.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/EditorTypes.hpp"
#include "spark/editor/panels/EditorDockShell.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/FlyCamera.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/Scene.hpp"

namespace Spark {

class GameObject;
class IEngineContext;
struct FrameTiming;

namespace Editor {

class HierarchyPanel;
class InspectorPanel;
class ProjectBrowserPanel;

/**
 * Central editor coordinator: scene bootstrap, UI shell, viewport camera, panel tick.
 * Used by spark_editor/EditorGame (implements IGame).
 */
class EditorApplication {
public:
    EditorApplication();
    ~EditorApplication();
    void OnAttach(Scene& scene, IEngineContext& context);
    void OnDetach(Scene& scene);
    void OnUpdate(const FrameTiming& timing, Scene& scene, IEngineContext& context);
    void OnRender(Scene& scene, IEngineContext& context);

    [[nodiscard]] EditorMode GetMode() const noexcept { return mode; }
    [[nodiscard]] const Utf8String& GetStatusLine() const noexcept { return statusLine; }

private:
    void BootstrapDefaultScene(GameWorld& world);
    void BuildEditorUi(GameWorld& world);
    void UpdateViewportCamera(const FrameTiming& timing, IEngineContext& context);
    void TickPanels(const FrameTiming& timing, Scene& scene, IEngineContext& engineContext);
    void HighlightSelection(Scene& scene);

    EditorMode mode = EditorMode::Edit;
    WorkspaceDimension workspace = WorkspaceDimension::ThreeD;
    EditorSelection selection{};
    EditorProject project{};
    EditorContext context{};
    EditorDockShell dock{};

    UniquePtr<HierarchyPanel> hierarchyPanel;
    UniquePtr<InspectorPanel> inspectorPanel;
    UniquePtr<ProjectBrowserPanel> projectPanel;

    FlyCamera viewportCamera{};
    GameObject* guiCanvasObject = nullptr;
    class GuiCanvasComponent* guiCanvas = nullptr;
    GameObject* fpsHudObject = nullptr;
    class TextOverlayComponent* fpsText = nullptr;
    GameObject* highlightedObject = nullptr;

    SharedPtr<Mesh> groundMesh;
    Utf8String statusLine{"Spark Editor — F1: fly camera · WASD move · RMB look"};
    bool uiBuilt = false;
};

}  // namespace Editor
}  // namespace Spark
