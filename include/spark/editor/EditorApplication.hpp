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

    [[nodiscard]] EditorMode GetMode() const noexcept { return mode_; }
    [[nodiscard]] const Utf8String& GetStatusLine() const noexcept { return statusLine_; }

private:
    void BootstrapDefaultScene(GameWorld& world);
    void BuildEditorUi(GameWorld& world);
    void UpdateViewportCamera(const FrameTiming& timing, IEngineContext& context);
    void TickPanels(const FrameTiming& timing, Scene& scene, IEngineContext& context);
    void HighlightSelection(Scene& scene);

    EditorMode mode_ = EditorMode::Edit;
    WorkspaceDimension workspace_ = WorkspaceDimension::ThreeD;
    EditorSelection selection_{};
    EditorProject project_{};
    EditorContext context_{};
    EditorDockShell dock_{};

    UniquePtr<HierarchyPanel> hierarchyPanel_;
    UniquePtr<InspectorPanel> inspectorPanel_;
    UniquePtr<ProjectBrowserPanel> projectPanel_;

    FlyCamera viewportCamera_{};
    GameObject* guiCanvasObject_ = nullptr;
    class GuiCanvasComponent* guiCanvas_ = nullptr;
    GameObject* fpsHudObject_ = nullptr;
    class TextOverlayComponent* fpsText_ = nullptr;
    GameObject* highlightedObject_ = nullptr;

    SharedPtr<Mesh> groundMesh_;
    Utf8String statusLine_{"Spark Editor — F1: fly camera · WASD move · RMB look"};
    bool uiBuilt_ = false;
};

}  // namespace Editor
}  // namespace Spark
