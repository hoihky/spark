#include "spark/editor/EditorApplication.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/PointLightComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/editor/EditorUiFont.hpp"
#include "spark/editor/panels/HierarchyPanel.hpp"
#include "spark/editor/panels/InspectorPanel.hpp"
#include "spark/editor/panels/ProjectBrowserPanel.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/gui/GuiScene.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/render/SceneGroundExtent.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/Window.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/ScenePartitionKind.hpp"
#include "spark/scene/SceneSubmit.hpp"

#include <GLFW/glfw3.h>

#include <cmath>

namespace Spark::Editor {

EditorApplication::EditorApplication() = default;

EditorApplication::~EditorApplication() = default;

void EditorApplication::BootstrapDefaultScene(GameWorld& world) {
    groundMesh_ = MakeShared<Mesh>(Utf8String("EditorGround"));
    *groundMesh_ = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    world.RegisterMesh(groundMesh_, "spark/editor/ground");

    GameObject* ground = world.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(
            groundMesh_, SceneMeshSlot::GroundPlane, Vector3{0.45F, 0.48F, 0.5F});

    GameObject* sun = world.CreateGameObject();
    sun->GetName() = Utf8String("Sun");
    TransformComponent* sunTr = sun->AddComponent<TransformComponent>();
    sunTr->SetTranslation({18.0F, 28.0F, 12.0F});
    sun->AddComponent<PointLightComponent>(Vector3{1.0F, 0.96F, 0.9F}, 2.4F, 120.0F);

    GameObject* sample = world.CreateGameObject();
    sample->GetName() = Utf8String("SampleCube");
    TransformComponent* sampleTr = sample->AddComponent<TransformComponent>();
    sampleTr->SetTranslation({0.0F, 0.5F, 0.0F});
    sampleTr->SetUniformScale(1.0F);
    sample->AddComponent<MeshComponent>(SharedPtr<Mesh>{}, SceneMeshSlot::UnitCube, Vector3{0.72F, 0.35F, 0.28F});
    if (MaterialComponent* mat = sample->AddComponent<MaterialComponent>()) {
        mat->SetMetallic(0.15F);
        mat->SetRoughness(0.42F);
    }
}

void EditorApplication::BuildEditorUi(GameWorld& world) {
    if (uiBuilt_) {
        return;
    }

    hierarchyPanel_ = MakeUnique<HierarchyPanel>();
    inspectorPanel_ = MakeUnique<InspectorPanel>();
    projectPanel_ = MakeUnique<ProjectBrowserPanel>();

    context_.world = &world;
    context_.selection = &selection_;
    context_.project = &project_;
    context_.mode = mode_;
    context_.workspace = workspace_;
    context_.statusLine = statusLine_;

    for (IEditorPanel* panel :
            {static_cast<IEditorPanel*>(hierarchyPanel_.Get()),
                    static_cast<IEditorPanel*>(inspectorPanel_.Get()),
                    static_cast<IEditorPanel*>(projectPanel_.Get())}) {
        panel->OnAttach(context_);
    }

    Gui::SceneEditorLayoutSettings layout{};
    (void)Gui::TryLoadSceneEditorLayout(layout);
    dock_.SetSidebarWidth(Gui::GetSceneEditorSidebarWidthPx());

    dock_.SetPanels(
            hierarchyPanel_->ReleaseRootWidget(),
            projectPanel_->ReleaseRootWidget(),
            inspectorPanel_->ReleaseRootWidget());

    guiCanvasObject_ = world.CreateGameObject();
    guiCanvasObject_->GetName() = Utf8String("EditorGui");
    guiCanvas_ = guiCanvasObject_->AddComponent<GuiCanvasComponent>();
    guiCanvas_->SetSortOrder(240);
    guiCanvas_->SetTheme(Gui::GuiTheme::SceneEditorDark());
    guiCanvas_->SetRoot(dock_.ReleaseRootWidget());

    fpsHudObject_ = world.CreateGameObject();
    fpsHudObject_->GetName() = Utf8String("EditorStatusHud");
    fpsText_ = fpsHudObject_->AddComponent<TextOverlayComponent>();
    fpsText_->SetScreenPosition(12.0F, 78.0F);
    fpsText_->SetFontSizePixels(16.0F);
    fpsText_->SetColor({0.9F, 0.93F, 0.98F});

    const Utf8String defaultProjectPath(SPARK_BUILD_ASSETS_DIR);
    (void)project_.OpenExisting(defaultProjectPath.CStr());

    uiBuilt_ = true;
}

void EditorApplication::UpdateViewportCamera(const FrameTiming& timing, IEngineContext& context) {
    IInput& input = context.GetInput();

    if (input.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        input.SetCursorCaptured(!input.IsCursorCaptured());
    }

    if (input.IsCursorCaptured()) {
        viewportCamera_.ProcessMovement(input, timing.deltaTimeSeconds);
        if (timing.frameIndex > 0) {
            viewportCamera_.AddLook(input.GetMouseDeltaX(), input.GetMouseDeltaY());
        }
        const float scroll = input.GetScrollDeltaY();
        if (std::fabs(scroll) > 1.0e-4F) {
            viewportCamera_.position += viewportCamera_.Forward() * (scroll * 0.65F);
        }
        return;
    }

    const float scroll = input.GetScrollDeltaY();
    if (std::fabs(scroll) > 1.0e-4F && !GuiScrollWheelConsumed()) {
        viewportCamera_.position += viewportCamera_.Forward() * (scroll * 0.65F);
    }

    if (GuiConsumesGamePointer()) {
        return;
    }

    if (timing.frameIndex > 0) {
        const bool rmbDown = input.IsMouseButtonDown(1);
        if (rmbDown) {
            viewportCamera_.AddLook(input.GetMouseDeltaX(), input.GetMouseDeltaY());
        }
    }

    viewportCamera_.ProcessMovement(input, timing.deltaTimeSeconds);
}

void EditorApplication::HighlightSelection(Scene& /*scene*/) {
    GameObject* selected = selection_.GetPrimary();
    if (highlightedObject_ != nullptr && highlightedObject_ != selected) {
        if (MaterialComponent* prev = highlightedObject_->GetComponent<MaterialComponent>()) {
            prev->SetEmissive(Vector3{}, 0.0F);
        }
        highlightedObject_ = nullptr;
    }
    if (selected == nullptr) {
        return;
    }
    if (MaterialComponent* mat = selected->GetComponent<MaterialComponent>()) {
        mat->SetEmissive({0.35F, 0.28F, 0.12F}, 0.45F);
        highlightedObject_ = selected;
    }
}

void EditorApplication::TickPanels(const FrameTiming& timing, Scene& scene, IEngineContext& context) {
    context_.world = &scene.GetWorld();
    context_.scene = &scene;
    context_.engine = &context;
    context_.statusLine = statusLine_;
    if (hierarchyPanel_) {
        hierarchyPanel_->OnTick(timing, context_);
    }
    if (inspectorPanel_) {
        inspectorPanel_->OnTick(timing, context_);
    }
    if (projectPanel_) {
        projectPanel_->OnTick(timing, context_);
    }
}

void EditorApplication::OnAttach(Scene& scene, IEngineContext& /*context*/) {
    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
    MountEditorUiFonts(scene.GetWorld());
    BootstrapDefaultScene(scene.GetWorld());
    BuildEditorUi(scene.GetWorld());
    viewportCamera_.position = {8.0F, 6.0F, 14.0F};
    viewportCamera_.SnapLookAt({0.0F, 0.0F, 0.0F});
}

void EditorApplication::OnDetach(Scene& scene) {
    GameWorld& world = scene.GetWorld();
    if (guiCanvasObject_ != nullptr) {
        world.DestroyGameObject(guiCanvasObject_);
        guiCanvasObject_ = nullptr;
        guiCanvas_ = nullptr;
    }
    if (fpsHudObject_ != nullptr) {
        world.DestroyGameObject(fpsHudObject_);
        fpsHudObject_ = nullptr;
        fpsText_ = nullptr;
    }
    if (hierarchyPanel_) {
        hierarchyPanel_->OnDetach();
    }
    if (inspectorPanel_) {
        inspectorPanel_->OnDetach();
    }
    if (projectPanel_) {
        projectPanel_->OnDetach();
    }
    hierarchyPanel_.Reset();
    inspectorPanel_.Reset();
    projectPanel_.Reset();
    uiBuilt_ = false;
}

void EditorApplication::OnUpdate(const FrameTiming& timing, Scene& scene, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    float contentScaleX = 1.0F;
    float contentScaleY = 1.0F;
    context.GetWindow().GetContentScale(contentScaleX, contentScaleY);
    ProcessGuiCanvasesInput(scene, context.GetInput(), fbW, fbH, contentScaleX, contentScaleY);

    UpdateViewportCamera(timing, context);
    TickPanels(timing, scene, context);
    if (fpsText_ != nullptr) {
        fpsText_->SetText(statusLine_);
    }
}

void EditorApplication::OnRender(Scene& scene, IEngineContext& context) {
    HighlightSelection(scene);

    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 500.0F);
    const Matrix4 view = viewportCamera_.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    const Vector3 lightDir = Vector3{0.35F, 0.88F, 0.32F}.Normalized();
    SceneRenderParams params{};
    FillStandardLitSceneFromWorld(
            scene.GetWorld(),
            context,
            viewProj,
            viewportCamera_.position,
            lightDir,
            Vector3{1.0F, 0.98F, 0.94F},
            0.95F,
            Vector3{0.11F, 0.12F, 0.14F},
            false,
            Vector3{1.0F, 0.0F, 0.0F},
            Vector3{0.0F, 1.0F, 0.0F},
            0.0F,
            params,
            SceneSpriteSortMode::SortOrderOnly,
            &scene);

    const Gui::Rect worldViewport = dock_.GetWorldViewportRect();
    if (worldViewport.width > 1.0F && worldViewport.height > 1.0F) {
        params.worldViewportScissorEnabled = true;
        params.worldViewportScissorX = worldViewport.x;
        params.worldViewportScissorY = worldViewport.y;
        params.worldViewportScissorW = worldViewport.width;
        params.worldViewportScissorH = worldViewport.height;
    }

    params.uiFont = scene.GetWorld().GetUiFont();
    params.uiBoldFont = scene.GetWorld().GetUiBoldFont();
    params.screenTexts.Clear();
    params.uiPaintOrderNext = 0U;
    PaintGuiCanvases(scene, params, fbW, fbH);

    scene.ForEachTextOverlay([&params](const TextOverlayComponent& tc) {
        ScreenTextDraw draw{};
        draw.text = tc.GetText();
        draw.x = tc.GetScreenX();
        draw.y = tc.GetScreenY();
        draw.sizePixels = tc.GetFontSizePixels();
        draw.color = tc.GetColor();
        draw.alpha = tc.GetAlpha();
        draw.paintOrder = params.NextUiPaintOrder();
        params.screenTexts.PushBack(MoveTemp(draw));
    });

    context.SetSceneRenderParams(params);
}

}  // namespace Spark::Editor
