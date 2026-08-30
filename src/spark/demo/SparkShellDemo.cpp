#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/DemoCatalog.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/demo/ThreeDDemo.hpp"
#include "spark/demo/ToonShadingDemo.hpp"
#include "spark/demo/MaterialShowcase3DDemo.hpp"
#include "spark/demo/GltfSamples3DDemo.hpp"
#include "spark/demo/ModelViewer3DDemo.hpp"
#include "spark/demo/SkyDemo.hpp"
#include "spark/demo/VfxShowcaseDemo.hpp"
#include "spark/demo/TerrainDemo.hpp"
#include "spark/demo/CharacterCameraDemo.hpp"
#include "spark/demo/Tetris2DDemo.hpp"
#include "spark/demo/Connect3Demo.hpp"
#include "spark/demo/SpaceInvaders2DDemo.hpp"
#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/BroadPhase2DDemo.hpp"
#include "spark/demo/RenderLayers2DDemo.hpp"
#include "spark/demo/TilemapShowcase2DDemo.hpp"
#include "spark/demo/ImGuiShowcaseDemo.hpp"
#include "spark/demo/Maze3DDemo.hpp"
#include "spark/demo/PhysicsBallThrow3DDemo.hpp"
#include "spark/demo/SteeringShowcase3DDemo.hpp"
#include "spark/demo/SceneEditor3DDemo.hpp"
#if SPARK_HAS_EDITOR
#include "spark/demo/SparkEditorDemo.hpp"
#endif
#include "spark/demo/TimeOfDayDemo.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"

#include <cstdio>

namespace Spark {

class ShellGame;

namespace {

struct LauncherThemeBinding {
    ShellGame* game = nullptr;
};

void LauncherThemePrev(void* userData);
void LauncherThemeNext(void* userData);
void LauncherDemoSelected(void* userData, int index);

}  // namespace


class ShellGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override {
        engineCtx = &context;
        Spark::Ui::SceneEditorLayoutSettings layout{};
        (void)Spark::Ui::TryLoadSceneEditorLayout(layout);
        Spark::Ui::SetActiveUiThemePreset(layout.guiTheme);
        DemoGui::ActivateDearImGuiDemoUi(context);
        MountUiFont(GetWorld());
        fpsOverlay.EnsureMounted(GetWorld());
        context.GetInput().SetCursorCaptured(false);
        BuildLauncherRetainedUi(GetWorld());
        ThreeDDemo::RequestGltfAssets(GetWorld());
    }

    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        float contentScaleX = 1.0F;
        float contentScaleY = 1.0F;
        context.GetWindow().GetContentScale(contentScaleX, contentScaleY);
        if (mode == DemoMode::Menu || mode == DemoMode::ImGuiShowcase) {
            context.GetInput().SetCursorCaptured(false);
        }
        Spark::ProcessUiCanvasesInput(GetScene(), context.GetInput(), fbW, fbH, contentScaleX, contentScaleY);

        fpsOverlay.SyncVisibilityFromGlobal();
        fpsOverlay.Update(timing, fbW);

        if (mode != DemoMode::Menu && mode != DemoMode::ImGuiShowcase && mode != DemoMode::ModelViewer3D) {
            DemoHelpHud::ProcessGlobalToggle(context.GetInput());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
                ReturnToMenu(context);
                return;
            }
        }

        if (mode == DemoMode::Menu && engineCtx != nullptr && pendingDemoLaunch >= 0) {
            const int idx = pendingDemoLaunch;
            pendingDemoLaunch = -1;
            EnterDemoByListIndex(idx);
        }

        if (mode == DemoMode::Menu && engineCtx != nullptr) {
            Spark::IInput& in = context.GetInput();
            DemoStorageId hotkeyId{};
            for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; ++key) {
                if (in.IsKeyPressedThisFrame(key) &&
                    DemoCatalog::TryResolveDigitHotkey(key, hotkeyId)) {
                    EnterDemoByStorageId(hotkeyId);
                    break;
                }
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_0) &&
                DemoCatalog::TryResolveDigitHotkey(GLFW_KEY_0, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            }
            if (DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_T, hotkeyId) &&
                in.IsKeyPressedThisFrame(GLFW_KEY_T)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_C) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_C, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_I) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_I, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_SEMICOLON) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_SEMICOLON, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Y) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_Y, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_B) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_B, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_N) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_N, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_G) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_G, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Q) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_Q, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_V) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_V, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
#if SPARK_HAS_EDITOR
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_E) &&
                       DemoCatalog::TryResolveLetterHotkey(GLFW_KEY_E, hotkeyId)) {
                EnterDemoByStorageId(hotkeyId);
#endif
            }
        }

        if (mode == DemoMode::ThreeD) {
            threeD.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Sky) {
            skyDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Particles) {
            vfxShowcaseDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Terrain) {
            terrainDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Character) {
            characterDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Tetris2D) {
            tetris2DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Connect3) {
            connect3Demo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::SpaceInvaders2D) {
            spaceInvaders2DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::RenderLayers2D) {
            renderLayers2DDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::TilemapShowcase2D) {
            tilemapShowcase2DDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Platformer2D) {
            platformer2DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::BroadPhase2D) {
            broadPhase2DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Maze3D) {
            maze3DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::PhysicsBall3D) {
            physicsBall3D.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::SteeringShowcase3D) {
            steeringShowcase3D.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::SceneEditor3D) {
            sceneEditor3D.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::ToonShading) {
            toonShadingDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::MaterialShowcase3D) {
            materialShowcase3DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::TimeOfDay) {
            timeOfDayDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::ImGuiShowcase) {
            imguiShowcaseDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::GltfSamples3D) {
            gltfSamples3DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::ModelViewer3D) {
            modelViewer3DDemo.Simulate(timing, context, GetWorld());
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
#if SPARK_HAS_EDITOR
        } else if (mode == DemoMode::SparkEditor) {
            sparkEditorDemo.Simulate(timing, GetScene(), context);
            if (!sparkEditorDemo.IsPlayActive() && context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
#endif
        }
        Game::OnUpdate(timing, context);
    }

    void OnRender(IRenderFrame& /*frame*/, IEngineContext& context) override {
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (mode == DemoMode::ThreeD) {
            threeD.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Sky) {
            skyDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Particles) {
            vfxShowcaseDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Terrain) {
            terrainDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Character) {
            characterDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Tetris2D) {
            tetris2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Connect3) {
            connect3Demo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::SpaceInvaders2D) {
            spaceInvaders2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::RenderLayers2D) {
            renderLayers2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::TilemapShowcase2D) {
            tilemapShowcase2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Platformer2D) {
            platformer2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::BroadPhase2D) {
            broadPhase2DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::Maze3D) {
            maze3DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::PhysicsBall3D) {
            physicsBall3D.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::SteeringShowcase3D) {
            steeringShowcase3D.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::SceneEditor3D) {
            sceneEditor3D.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::ToonShading) {
            toonShadingDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::MaterialShowcase3D) {
            materialShowcase3DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::TimeOfDay) {
            timeOfDayDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::ImGuiShowcase) {
            imguiShowcaseDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::GltfSamples3D) {
            gltfSamples3DDemo.Render(GetScene(), GetWorld(), context);
        } else if (mode == DemoMode::ModelViewer3D) {
            modelViewer3DDemo.Render(GetScene(), GetWorld(), context);
#if SPARK_HAS_EDITOR
        } else if (mode == DemoMode::SparkEditor) {
            sparkEditorDemo.Render(GetScene(), context);
#endif
        } else {
            RenderUiOnly(context, fbW, fbH);
        }
        fpsOverlay.PatchSceneRenderParams(context);
    }

    void EnterThreeD(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!threeDLoaded) {
            threeD.Load(GetWorld(), context);
            threeDLoaded = true;
        }
        mode = DemoMode::ThreeD;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterToonShadingDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!toonShadingLoaded) {
            toonShadingDemo.Load(GetWorld(), context);
            toonShadingLoaded = true;
        }
        mode = DemoMode::ToonShading;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterMaterialShowcase3DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!materialShowcaseLoaded) {
            materialShowcase3DDemo.Load(GetWorld(), context);
            materialShowcaseLoaded = true;
        }
        mode = DemoMode::MaterialShowcase3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterSkyDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!skyDemoLoaded) {
            skyDemo.Load(GetWorld(), context);
            skyDemoLoaded = true;
        }
        mode = DemoMode::Sky;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterTimeOfDayDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!timeOfDayDemoLoaded) {
            timeOfDayDemo.Load(GetWorld(), context);
            timeOfDayDemoLoaded = true;
        }
        mode = DemoMode::TimeOfDay;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterParticleDemo(IEngineContext& context) {
        DemoGui::ActivateDearImGuiDemoUi(context);
        UnloadAllActiveDemos(context);
        if (!vfxShowcaseDemoLoaded) {
            vfxShowcaseDemo.Load(GetWorld(), context);
            vfxShowcaseDemoLoaded = true;
        }
        mode = DemoMode::Particles;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterTerrainDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!terrainDemoLoaded) {
            terrainDemo.Load(GetWorld(), context);
            terrainDemoLoaded = true;
        }
        mode = DemoMode::Terrain;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterCharacterDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!characterDemoLoaded) {
            characterDemo.Load(GetWorld(), context);
            characterDemoLoaded = true;
        }
        mode = DemoMode::Character;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterTetris2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!tetris2DLoaded) {
            tetris2DDemo.Load(GetWorld(), context);
            tetris2DLoaded = true;
        }
        mode = DemoMode::Tetris2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterConnect3Demo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!connect3Loaded) {
            connect3Demo.Load(GetWorld(), context);
            connect3Loaded = true;
        }
        mode = DemoMode::Connect3;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterSpaceInvaders2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Load(GetWorld(), context);
            spaceInvaders2DLoaded = true;
        }
        mode = DemoMode::SpaceInvaders2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterPlatformer2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        platformer2DDemo.Load(GetWorld(), context);
        platformer2DLoaded = true;
        mode = DemoMode::Platformer2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterBroadPhase2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        broadPhase2DDemo.Load(GetWorld(), context);
        broadPhase2DLoaded = true;
        mode = DemoMode::BroadPhase2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterRenderLayers2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        renderLayers2DDemo.Load(GetWorld(), context);
        renderLayers2DLoaded = true;
        mode = DemoMode::RenderLayers2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterTilemapShowcase2DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        tilemapShowcase2DDemo.Load(GetWorld(), context);
        tilemapShowcase2DLoaded = true;
        mode = DemoMode::TilemapShowcase2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterImGuiShowcase(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        imguiShowcaseDemo.Enter(context);
        mode = DemoMode::ImGuiShowcase;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterGltfSamples3DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        if (!gltfSamplesLoaded) {
            gltfSamples3DDemo.Load(GetWorld(), context);
            gltfSamplesLoaded = true;
        }
        mode = DemoMode::GltfSamples3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterModelViewer3DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        modelViewer3DDemo.Load(GetWorld(), context);
        modelViewer3DLoaded = true;
        mode = DemoMode::ModelViewer3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterMaze3DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        maze3DDemo.Load(GetWorld(), context);
        maze3DLoaded = true;
        mode = DemoMode::Maze3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterPhysicsBall3DDemo(IEngineContext& context) {
        DemoGui::ActivateDearImGuiDemoUi(context);
        if (threeDLoaded) {
            threeD.Unload(GetWorld());
            threeDLoaded = false;
        }
        if (skyDemoLoaded) {
            skyDemo.Unload(GetWorld());
            skyDemoLoaded = false;
        }
        if (timeOfDayDemoLoaded) {
            timeOfDayDemo.Unload(GetWorld());
            timeOfDayDemoLoaded = false;
        }
        if (vfxShowcaseDemoLoaded) {
            vfxShowcaseDemo.Unload(GetWorld());
            vfxShowcaseDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
        }
        if (tetris2DLoaded) {
            tetris2DDemo.Unload(GetWorld());
            tetris2DLoaded = false;
        }
        if (connect3Loaded) {
            connect3Demo.Unload(GetWorld());
            connect3Loaded = false;
        }
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        if (platformer2DLoaded) {
            platformer2DDemo.Unload(GetWorld());
            platformer2DLoaded = false;
        }
        if (broadPhase2DLoaded) {
            broadPhase2DDemo.Unload(GetWorld());
            broadPhase2DLoaded = false;
        }
        if (renderLayers2DLoaded) {
            renderLayers2DDemo.Unload(GetWorld());
            renderLayers2DLoaded = false;
        }
        if (tilemapShowcase2DLoaded) {
            tilemapShowcase2DDemo.Unload(GetWorld());
            tilemapShowcase2DLoaded = false;
        }
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        UnloadAllActiveDemos(context);
        physicsBall3D.Load(GetWorld(), context);
        physicsBall3DLoaded = true;
        mode = DemoMode::PhysicsBall3D;
        /** <c>PhysicsBallThrow3DDemo::Load</c> leaves capture off for LMB throw + right panel; use F1 to look. */
    }

    void EnterSteeringShowcase3DDemo(IEngineContext& context) {
        if (threeDLoaded) {
            threeD.Unload(GetWorld());
            threeDLoaded = false;
        }
        if (skyDemoLoaded) {
            skyDemo.Unload(GetWorld());
            skyDemoLoaded = false;
        }
        if (timeOfDayDemoLoaded) {
            timeOfDayDemo.Unload(GetWorld());
            timeOfDayDemoLoaded = false;
        }
        if (vfxShowcaseDemoLoaded) {
            vfxShowcaseDemo.Unload(GetWorld());
            vfxShowcaseDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
        }
        if (tetris2DLoaded) {
            tetris2DDemo.Unload(GetWorld());
            tetris2DLoaded = false;
        }
        if (connect3Loaded) {
            connect3Demo.Unload(GetWorld());
            connect3Loaded = false;
        }
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        if (platformer2DLoaded) {
            platformer2DDemo.Unload(GetWorld());
            platformer2DLoaded = false;
        }
        if (broadPhase2DLoaded) {
            broadPhase2DDemo.Unload(GetWorld());
            broadPhase2DLoaded = false;
        }
        if (renderLayers2DLoaded) {
            renderLayers2DDemo.Unload(GetWorld());
            renderLayers2DLoaded = false;
        }
        if (tilemapShowcase2DLoaded) {
            tilemapShowcase2DDemo.Unload(GetWorld());
            tilemapShowcase2DLoaded = false;
        }
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        UnloadAllActiveDemos(context);
        steeringShowcase3D.Load(GetWorld(), context);
        steeringShowcase3DLoaded = true;
        mode = DemoMode::SteeringShowcase3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterSceneEditor3DDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        sceneEditor3D.Load(GetWorld(), context);
        sceneEditor3DLoaded = true;
        mode = DemoMode::SceneEditor3D;
        context.GetInput().SetCursorCaptured(false);
    }

#if SPARK_HAS_EDITOR
    void EnterSparkEditorDemo(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
        sparkEditorDemo.Load(GetScene(), context);
        sparkEditorDemoLoaded = true;
        mode = DemoMode::SparkEditor;
        context.GetInput().SetCursorCaptured(false);
    }
#endif

    void ReturnToMenu(IEngineContext& context) {
        UnloadAllActiveDemos(context);
        mode = DemoMode::Menu;
        launcherSelectedIndex = -1;
        pendingDemoLaunch = -1;
        context.GetInput().SetCursorCaptured(false);
        BuildLauncherRetainedUi(GetWorld());
        SetLauncherCanvasEnabled(true);
    }

    void OnLauncherListSelected(const int index) {
        launcherSelectedIndex = index;
        pendingDemoLaunch = index;
    }

    void CycleShellTheme(const int delta) {
        const int n = Ui::UiThemePresetCount();
        int cur = static_cast<int>(Ui::GetActiveUiThemePreset());
        cur = ((cur + delta) % n + n) % n;
        OnShellThemeSelected(cur);
    }

private:
    void UnloadAllActiveDemos(IEngineContext& context) {
        if (mode == DemoMode::ImGuiShowcase) {
            imguiShowcaseDemo.Leave(context, GetWorld());
        }
        context.SetSceneRenderParams(SceneRenderParams{});
        if (threeDLoaded) {
            threeD.Unload(GetWorld());
            threeDLoaded = false;
        }
        if (skyDemoLoaded) {
            skyDemo.Unload(GetWorld());
            skyDemoLoaded = false;
        }
        if (timeOfDayDemoLoaded) {
            timeOfDayDemo.Unload(GetWorld());
            timeOfDayDemoLoaded = false;
        }
        if (vfxShowcaseDemoLoaded) {
            vfxShowcaseDemo.Unload(GetWorld());
            vfxShowcaseDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
        }
        if (tetris2DLoaded) {
            tetris2DDemo.Unload(GetWorld());
            tetris2DLoaded = false;
        }
        if (connect3Loaded) {
            connect3Demo.Unload(GetWorld());
            connect3Loaded = false;
        }
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        if (platformer2DLoaded) {
            platformer2DDemo.Unload(GetWorld());
            platformer2DLoaded = false;
        }
        if (broadPhase2DLoaded) {
            broadPhase2DDemo.Unload(GetWorld());
            broadPhase2DLoaded = false;
        }
        if (renderLayers2DLoaded) {
            renderLayers2DDemo.Unload(GetWorld());
            renderLayers2DLoaded = false;
        }
        if (tilemapShowcase2DLoaded) {
            tilemapShowcase2DDemo.Unload(GetWorld());
            tilemapShowcase2DLoaded = false;
        }
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        if (physicsBall3DLoaded) {
            physicsBall3D.Unload(GetWorld());
            physicsBall3DLoaded = false;
        }
        if (steeringShowcase3DLoaded) {
            steeringShowcase3D.Unload(GetWorld());
            steeringShowcase3DLoaded = false;
        }
        if (sceneEditor3DLoaded) {
            sceneEditor3D.Unload(GetWorld());
            sceneEditor3DLoaded = false;
        }
        if (toonShadingLoaded) {
            toonShadingDemo.Unload(GetWorld());
            toonShadingLoaded = false;
        }
        if (materialShowcaseLoaded) {
            materialShowcase3DDemo.Unload(GetWorld());
            materialShowcaseLoaded = false;
        }
        if (gltfSamplesLoaded) {
            gltfSamples3DDemo.Unload(GetWorld());
            gltfSamplesLoaded = false;
        }
        if (modelViewer3DLoaded) {
            modelViewer3DDemo.Unload(GetWorld());
            modelViewer3DLoaded = false;
        }
#if SPARK_HAS_EDITOR
        if (sparkEditorDemoLoaded) {
            sparkEditorDemo.Unload(GetScene());
            sparkEditorDemoLoaded = false;
        }
#endif
        GetScene().SetSpatialPartitionKind(Spark::ScenePartitionKind::None);
    }

    void EnterDemoByListIndex(const int launcherIndex) {
        if (launcherIndex < 0 ||
            launcherIndex >= static_cast<int>(DemoCatalog::LauncherRowCount())) {
            return;
        }
        EnterDemoByStorageId(DemoCatalog::LauncherStorageId(static_cast<std::size_t>(launcherIndex)));
    }

    void EnterDemoByStorageId(const DemoStorageId id) {
        if (engineCtx == nullptr) {
            return;
        }
        EnterDemoByStorageIndex(static_cast<int>(id));
    }

    void EnterDemoByStorageIndex(const int storageIndex) {
        if (engineCtx == nullptr) {
            return;
        }
        using EnterFn = void (ShellGame::*)(IEngineContext&);
        static constexpr EnterFn kDemoEnter[] = {
                &ShellGame::EnterThreeD,
                &ShellGame::EnterSkyDemo,
                &ShellGame::EnterParticleDemo,
                &ShellGame::EnterTerrainDemo,
                &ShellGame::EnterCharacterDemo,
                &ShellGame::EnterPlatformer2DDemo,
                &ShellGame::EnterBroadPhase2DDemo,
                &ShellGame::EnterMaze3DDemo,
                &ShellGame::EnterPhysicsBall3DDemo,
                &ShellGame::EnterSceneEditor3DDemo,
                &ShellGame::EnterTetris2DDemo,
                &ShellGame::EnterConnect3Demo,
                &ShellGame::EnterSpaceInvaders2DDemo,
                &ShellGame::EnterSteeringShowcase3DDemo,
                &ShellGame::EnterToonShadingDemo,
                &ShellGame::EnterMaterialShowcase3DDemo,
                &ShellGame::EnterTimeOfDayDemo,
                &ShellGame::EnterRenderLayers2DDemo,
                &ShellGame::EnterTilemapShowcase2DDemo,
                &ShellGame::EnterImGuiShowcase,
                &ShellGame::EnterGltfSamples3DDemo,
                &ShellGame::EnterModelViewer3DDemo,
#if SPARK_HAS_EDITOR
                &ShellGame::EnterSparkEditorDemo,
#endif
        };
        static_assert(
                sizeof(kDemoEnter) / sizeof(kDemoEnter[0]) == static_cast<std::size_t>(DemoStorageId::Count),
                "kDemoEnter must match launcher demo list count");
        if (storageIndex < 0 ||
            storageIndex >= static_cast<int>(DemoCatalog::StorageCount())) {
            return;
        }
        SetLauncherCanvasEnabled(false);
        DestroyLauncherRetainedUi(GetWorld());
        (this->*kDemoEnter[storageIndex])(*engineCtx);
    }

    void DestroyLauncherRetainedUi(GameWorld& world) {
        if (launcherUiRoot != nullptr) {
            world.DestroyGameObject(launcherUiRoot);
            launcherUiRoot = nullptr;
            launcherCanvas = nullptr;
            launcherThemeLabel = nullptr;
            launcherList = nullptr;
        }
    }

    void BuildLauncherRetainedUi(GameWorld& world) {
        if (launcherCanvas != nullptr) {
            return;
        }
        launcherUiRoot = world.CreateGameObject();
        launcherUiRoot->GetName() = Utf8String("ShellLauncherUi");
        launcherCanvas = launcherUiRoot->AddComponent<UiCanvasComponent>();
        launcherCanvas->SetSortOrder(200);
        launcherCanvas->SetTheme(Ui::UiTheme::ClassicMint());

        Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

        Ui::PanelDesc panelDesc{};
        panelDesc.id = Utf8String("launcher");
        panelDesc.title = Utf8String("Spark Demo Launcher");
        panelDesc.width = DemoGui::kDemoLauncherPanelWidth;
        panelDesc.height = 680.0F;
        panelDesc.centerInParent = true;
        auto panel = factory.CreatePanel(panelDesc);



        Ui::LabelDesc helpDesc{};
        helpDesc.id = Utf8String("help");
        helpDesc.text = Utf8String(
                "* = recommended. Demos listed 1-22. Hotkeys: 1-9, 0, T, C, I, ;, Y, B, N, G, Q, V. H toggles help.");
        helpDesc.muted = true;
        auto help = factory.CreateLabel(helpDesc);

        Ui::ListDesc listDesc{};
        listDesc.id = Utf8String("demo_list");
        listDesc.rowHeight = 28.0F;
        listDesc.itemFontSize = 16.0F;
        listDesc.verticalScrollingEnabled = true;
        listDesc.fillRemainingHeight = true;

        auto list = factory.CreateList(listDesc);
        launcherList = list.Get();
        Array<Utf8String> items;
        items.Reserve(DemoCatalog::LauncherRowCount());
        for (std::size_t i = 0; i < DemoCatalog::LauncherRowCount(); ++i) {
            items.PushBack(Utf8String(DemoCatalog::LauncherRowLabel(i)));
        }
        list->SetItems(MoveTemp(items));
        if (launcherSelectedIndex >= 0) {
            list->SetSelectedIndex(launcherSelectedIndex);
        }
        Ui::UiIntCallback selectCb{};
        selectCb.fn = &LauncherDemoSelected;
        selectCb.userData = this;
        list->SetOnSelectionChanged(selectCb);


        AdoptUiChild(*panel, MoveTemp(help));
        AdoptUiChild(*panel, MoveTemp(list));

        launcherCanvas->SetRoot(MoveTemp(panel));
    }

    void SyncLauncherThemeLabel() {
        if (launcherThemeLabel != nullptr) {
            launcherThemeLabel->SetText(
                    Utf8String(Ui::GetUiThemePresetDisplayName(Ui::GetActiveUiThemePreset())));
        }
    }

    void SetLauncherCanvasEnabled(const bool enabled) noexcept {
        if (launcherCanvas != nullptr) {
            launcherCanvas->SetCanvasEnabled(enabled);
        }
    }

    void BuildLauncherPortableUi(SceneRenderParams& params, const int fbW, const int fbH) {
        PaintUiCanvases(GetWorld(), params, fbW, fbH);
    }

    void SaveShellGuiPreferences() {
        Spark::Ui::SceneEditorLayoutSettings layout{};
        (void)Spark::Ui::TryLoadSceneEditorLayout(layout);
        layout.guiTheme = Spark::Ui::GetActiveUiThemePreset();
        (void)Spark::Ui::SaveSceneEditorLayout(layout);
    }

    void OnShellThemeSelected(const int idx) {
        Spark::Ui::SetActiveUiThemePreset(Spark::Ui::UiThemePresetFromId(idx));
        SaveShellGuiPreferences();
        SyncLauncherThemeLabel();
    }

    void RenderUiOnly(IEngineContext& context, int fbW, int fbH) {
        if (fbW <= 0) {
            fbW = 1;
        }
        if (fbH <= 0) {
            fbH = 1;
        }
        const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = Spark::Matrix4::Identity;
        Spark::SceneRenderParams params{};
        params.viewProjection = proj * view;
        params.cameraPositionWorld = {0.0F, 2.0F, 8.0F};
        params.lightDirectionWorld = Spark::Vector3{0.3F, 0.85F, 0.4F}.Normalized();
        params.lightColor = {1.0F, 1.0F, 1.0F};
        params.lightIntensity = 0.0F;
        const Spark::Ui::UiTheme menuSkin =
                Spark::Ui::ResolveUiTheme(Spark::Ui::GetActiveUiThemePreset());
        params.ambientColor = {
                menuSkin.shellBackdropBottom.x * 0.14F,
                menuSkin.shellBackdropBottom.y * 0.14F,
                menuSkin.shellBackdropBottom.z * 0.14F};
        params.uiFont = GetWorld().GetUiFont();
        params.uiBoldFont = GetWorld().GetUiBoldFont();
        if (mode == DemoMode::Menu) {
            BuildLauncherPortableUi(params, fbW, fbH);
        }
        context.SetSceneRenderParams(params);
    }

    IEngineContext* engineCtx = nullptr;

    GameObject* launcherUiRoot = nullptr;
    UiCanvasComponent* launcherCanvas = nullptr;
    Ui::ILabel* launcherThemeLabel = nullptr;
    Ui::IList* launcherList = nullptr;
    LauncherThemeBinding themePrevBinding{};
    LauncherThemeBinding themeNextBinding{};

    DemoMode mode = DemoMode::Menu;
    int launcherSelectedIndex = -1;
    int pendingDemoLaunch = -1;
    ThreeDDemo threeD{};
    bool threeDLoaded = false;
    SkyDemo skyDemo{};
    bool skyDemoLoaded = false;
    VfxShowcaseDemo vfxShowcaseDemo{};
    bool vfxShowcaseDemoLoaded = false;
    TerrainDemo terrainDemo{};
    bool terrainDemoLoaded = false;
    CharacterCameraDemo characterDemo{};
    bool characterDemoLoaded = false;
    Tetris2DDemo tetris2DDemo{};
    bool tetris2DLoaded = false;
    Connect3Demo connect3Demo{};
    bool connect3Loaded = false;
    SpaceInvaders2DDemo spaceInvaders2DDemo{};
    bool spaceInvaders2DLoaded = false;
    Platformer2DDemo platformer2DDemo{};
    bool platformer2DLoaded = false;
    BroadPhase2DDemo broadPhase2DDemo{};
    bool broadPhase2DLoaded = false;
    RenderLayers2DDemo renderLayers2DDemo{};
    bool renderLayers2DLoaded = false;
    TilemapShowcase2DDemo tilemapShowcase2DDemo{};
    bool tilemapShowcase2DLoaded = false;
    Maze3DDemo maze3DDemo{};
    bool maze3DLoaded = false;
    PhysicsBallThrow3DDemo physicsBall3D{};
    bool physicsBall3DLoaded = false;
    SteeringShowcase3DDemo steeringShowcase3D{};
    bool steeringShowcase3DLoaded = false;
    SceneEditor3DDemo sceneEditor3D{};
    bool sceneEditor3DLoaded = false;
    ToonShadingDemo toonShadingDemo{};
    bool toonShadingLoaded = false;
    MaterialShowcase3DDemo materialShowcase3DDemo{};
    bool materialShowcaseLoaded = false;
    TimeOfDayDemo timeOfDayDemo{};
    bool timeOfDayDemoLoaded = false;
    ImGuiShowcaseDemo imguiShowcaseDemo{};
    GltfSamples3DDemo gltfSamples3DDemo{};
    bool gltfSamplesLoaded = false;
    ModelViewer3DDemo modelViewer3DDemo{};
    bool modelViewer3DLoaded = false;
#if SPARK_HAS_EDITOR
    SparkEditorDemo sparkEditorDemo{};
    bool sparkEditorDemoLoaded = false;
#endif
    DemoFpsToggleOverlay fpsOverlay{};
};

namespace {

void LauncherThemePrev(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* binding = static_cast<LauncherThemeBinding*>(userData);
    if (binding->game != nullptr) {
        binding->game->CycleShellTheme(-1);
    }
}

void LauncherThemeNext(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* binding = static_cast<LauncherThemeBinding*>(userData);
    if (binding->game != nullptr) {
        binding->game->CycleShellTheme(1);
    }
}

void LauncherDemoSelected(void* userData, const int index) {
    if (userData == nullptr) {
        return;
    }
    static_cast<ShellGame*>(userData)->OnLauncherListSelected(index);
}

}  // namespace

UniquePtr<IGame> NewShellDemoGame() {
    return Engine::NewGame<ShellGame>();
}

}  // namespace Spark
