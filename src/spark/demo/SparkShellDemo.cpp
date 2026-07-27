#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/gui/api/GuiApi.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/demo/ThreeDDemo.hpp"
#include "spark/demo/ToonShadingDemo.hpp"
#include "spark/demo/MaterialShowcase3DDemo.hpp"
#include "spark/demo/SkyDemo.hpp"
#include "spark/demo/ParticleDemo.hpp"
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
#include "spark/demo/TimeOfDayDemo.hpp"
#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"
#include "spark/gui/toolkit/GuiToolkitSettings.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"

#include <cstdio>

namespace Spark {


class ShellGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override {
        engineCtx = &context;
        Spark::Gui::SceneEditorLayoutSettings layout{};
        Spark::Gui::TryLoadSceneEditorLayout(layout);
        Spark::Gui::SetActiveGuiThemePreset(layout.guiTheme);
        MountUiFont(GetWorld());
        fpsOverlay.EnsureMounted(GetWorld());
        context.GetInput().SetCursorCaptured(false);
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
        Spark::ProcessGuiCanvasesInput(GetScene(), context.GetInput(), fbW, fbH, contentScaleX, contentScaleY);

        fpsOverlay.SyncVisibilityFromGlobal();
        fpsOverlay.Update(timing, fbW);

        if (mode == DemoMode::Menu && engineCtx != nullptr && pendingDemoLaunch >= 0) {
            const int idx = pendingDemoLaunch;
            pendingDemoLaunch = -1;
            EnterDemoByListIndex(idx);
        }

        if (mode == DemoMode::Menu && engineCtx != nullptr) {
            Spark::IInput& in = context.GetInput();
            for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; ++key) {
                if (in.IsKeyPressedThisFrame(key)) {
                    EnterDemoByListIndex(key - GLFW_KEY_1);
                }
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_0)) {
                EnterDemoByListIndex(9);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_T)) {
                EnterDemoByListIndex(10);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_C)) {
                EnterDemoByListIndex(11);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_I)) {
                EnterDemoByListIndex(12);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_SEMICOLON)) {
                EnterDemoByListIndex(13);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Y)) {
                EnterDemoByListIndex(14);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_B)) {
                EnterDemoByListIndex(15);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_N)) {
                EnterDemoByListIndex(16);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_G)) {
                EnterDemoByListIndex(19);
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
            particleDemo.Simulate(timing, context);
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
            tetris2DDemo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::Connect3) {
            connect3Demo.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::SpaceInvaders2D) {
            spaceInvaders2DDemo.Simulate(timing, context);
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
            materialShowcase3DDemo.Simulate(timing, context);
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
            particleDemo.Render(GetScene(), GetWorld(), context);
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
        } else {
            RenderUiOnly(context, fbW, fbH);
        }
        fpsOverlay.PatchSceneRenderParams(context);
    }

    void EnterThreeD(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
        if (skyDemoLoaded) {
            skyDemo.Unload(GetWorld());
            skyDemoLoaded = false;
        }
        if (timeOfDayDemoLoaded) {
            timeOfDayDemo.Unload(GetWorld());
            timeOfDayDemoLoaded = false;
        }
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!threeDLoaded) {
            threeD.Load(GetWorld(), context);
            threeDLoaded = true;
        }
        mode = DemoMode::ThreeD;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterToonShadingDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!toonShadingLoaded) {
            toonShadingDemo.Load(GetWorld(), context);
            toonShadingLoaded = true;
        }
        mode = DemoMode::ToonShading;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterMaterialShowcase3DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!materialShowcaseLoaded) {
            materialShowcase3DDemo.Load(GetWorld(), context);
            materialShowcaseLoaded = true;
        }
        mode = DemoMode::MaterialShowcase3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterSkyDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
        if (threeDLoaded) {
            threeD.Unload(GetWorld());
            threeDLoaded = false;
        }
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!skyDemoLoaded) {
            skyDemo.Load(GetWorld(), context);
            skyDemoLoaded = true;
        }
        mode = DemoMode::Sky;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterTimeOfDayDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (toonShadingLoaded) {
            toonShadingDemo.Unload(GetWorld());
            toonShadingLoaded = false;
        }
        if (materialShowcaseLoaded) {
            materialShowcase3DDemo.Unload(GetWorld());
            materialShowcaseLoaded = false;
        }
        if (!timeOfDayDemoLoaded) {
            timeOfDayDemo.Load(GetWorld(), context);
            timeOfDayDemoLoaded = true;
        }
        mode = DemoMode::TimeOfDay;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterParticleDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (!particleDemoLoaded) {
            particleDemo.Load(GetWorld(), context);
            particleDemoLoaded = true;
        }
        mode = DemoMode::Particles;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterTerrainDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!terrainDemoLoaded) {
            terrainDemo.Load(GetWorld(), context);
            terrainDemoLoaded = true;
        }
        mode = DemoMode::Terrain;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterCharacterDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
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
        if (!characterDemoLoaded) {
            characterDemo.Load(GetWorld(), context);
            characterDemoLoaded = true;
        }
        mode = DemoMode::Character;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterTetris2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
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
        if (connect3Loaded) {
            connect3Demo.Unload(GetWorld());
            connect3Loaded = false;
        }
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        if (!tetris2DLoaded) {
            tetris2DDemo.Load(GetWorld(), context);
            tetris2DLoaded = true;
        }
        mode = DemoMode::Tetris2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterConnect3Demo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
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
        if (tetris2DLoaded) {
            tetris2DDemo.Unload(GetWorld());
            tetris2DLoaded = false;
        }
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        if (!connect3Loaded) {
            connect3Demo.Load(GetWorld(), context);
            connect3Loaded = true;
        }
        mode = DemoMode::Connect3;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterSpaceInvaders2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
        }
        if (terrainDemoLoaded) {
            terrainDemo.Unload(GetWorld());
            terrainDemoLoaded = false;
        }
        if (characterDemoLoaded) {
            characterDemo.Unload(GetWorld());
            characterDemoLoaded = false;
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
        if (tetris2DLoaded) {
            tetris2DDemo.Unload(GetWorld());
            tetris2DLoaded = false;
        }
        if (connect3Loaded) {
            connect3Demo.Unload(GetWorld());
            connect3Loaded = false;
        }
        if (!spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Load(GetWorld(), context);
            spaceInvaders2DLoaded = true;
        }
        mode = DemoMode::SpaceInvaders2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterPlatformer2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        platformer2DDemo.Load(GetWorld(), context);
        platformer2DLoaded = true;
        mode = DemoMode::Platformer2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterBroadPhase2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        broadPhase2DDemo.Load(GetWorld(), context);
        broadPhase2DLoaded = true;
        mode = DemoMode::BroadPhase2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterRenderLayers2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!renderLayers2DLoaded) {
            renderLayers2DDemo.Load(GetWorld(), context);
            renderLayers2DLoaded = true;
        }
        mode = DemoMode::RenderLayers2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterTilemapShowcase2DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        if (!tilemapShowcase2DLoaded) {
            tilemapShowcase2DDemo.Load(GetWorld(), context);
            tilemapShowcase2DLoaded = true;
        }
        mode = DemoMode::TilemapShowcase2D;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterImGuiShowcase(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        imguiShowcaseDemo.Enter(context);
        mode = DemoMode::ImGuiShowcase;
        context.GetInput().SetCursorCaptured(false);
    }

    void EnterMaze3DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        maze3DDemo.Load(GetWorld(), context);
        maze3DLoaded = true;
        mode = DemoMode::Maze3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterPhysicsBall3DDemo(IEngineContext& context) {
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
        steeringShowcase3D.Load(GetWorld(), context);
        steeringShowcase3DLoaded = true;
        mode = DemoMode::SteeringShowcase3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterSceneEditor3DDemo(IEngineContext& context) {
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        sceneEditor3D.Load(GetWorld(), context);
        sceneEditor3DLoaded = true;
        mode = DemoMode::SceneEditor3D;
        context.GetInput().SetCursorCaptured(false);
    }

    void ReturnToMenu(IEngineContext& context) {
        if (mode == DemoMode::ImGuiShowcase) {
            imguiShowcaseDemo.Leave(context);
        }
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
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
        if (particleDemoLoaded) {
            particleDemo.Unload(GetWorld());
            particleDemoLoaded = false;
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
        GetScene().SetSpatialPartitionKind(Spark::ScenePartitionKind::None);
        mode = DemoMode::Menu;
        launcherSelectedIndex = -1;
        pendingDemoLaunch = -1;
        context.GetInput().SetCursorCaptured(false);
    }

private:
    void UnloadTimeOfDayDemoIfAny() {
        if (timeOfDayDemoLoaded) {
            timeOfDayDemo.Unload(GetWorld());
            timeOfDayDemoLoaded = false;
        }
    }

    void UnloadPhysicsBall3DDemoIfAny() {
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
        if (materialShowcaseLoaded) {
            materialShowcase3DDemo.Unload(GetWorld());
            materialShowcaseLoaded = false;
        }
        if (toonShadingLoaded) {
            toonShadingDemo.Unload(GetWorld());
            toonShadingLoaded = false;
        }
    }

    void EnterDemoByListIndex(const int idx) {
        if (engineCtx == nullptr) {
            return;
        }
        constexpr int kImGuiShowcaseIndex = 19;
        if (idx != kImGuiShowcaseIndex) {
            Spark::Gui::GuiToolkitSettings::SetPreferred(Spark::Gui::GuiToolkitKind::SparkNative);
            if (IImGuiLayer* layer = engineCtx->TryGetImGuiLayer()) {
                layer->SetEnabled(false);
            }
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
        };
        static_assert(
                sizeof(kDemoEnter) / sizeof(kDemoEnter[0]) == 20,
                "kDemoEnter must match launcher demo list count");
        if (idx < 0 || idx >= static_cast<int>(sizeof(kDemoEnter) / sizeof(kDemoEnter[0]))) {
            return;
        }
        (this->*kDemoEnter[idx])(*engineCtx);
    }

    void BuildLauncherPortableUi(IEngineContext& context, SceneRenderParams& params) {
        static constexpr const char* kDemoLabels[] = {
                "1  — Basic 3D scene",
                "2  — Sky: box, dome, plane",
                "3  — Particles",
                "4  — Terrain",
                "5  — Character (1st / 3rd person)",
                "6  — 2D platformer",
                "7  — 2D Maze",
                "8  — 3D maze",
                "9  — 3D physics",
                "10 — 3D scene editor",
                "11 — Tetris",
                "12 — Match-3",
                "13 — Space Invaders",
                "14 — 3D steering",
                "15 — Toon/cel shading",
                "16 — Material ball",
                "17 — Time of day",
                "18 — Farming RPG render layers",
                "19 — Tilemap layers, animation & pathfinding",
                "20 — Dear ImGui tools (docking)",
        };
        constexpr int kDemoCount = static_cast<int>(sizeof(kDemoLabels) / sizeof(kDemoLabels[0]));

        const Gui::GuiFrameContext frame = DemoGui::MakeFrameContext(context, params, GetWorld(), 0.0F);
        Gui::GuiSystem::Get().BeginImmediateFrame(frame);
        Gui::IGuiFrame& ui = Gui::Ui();

        const Gui::GuiLayoutMetrics& layout = Gui::GetActiveGuiLayoutMetrics();
        const float fbW = static_cast<float>((std::max)(1, frame.framebufferWidth));
        const float fbH = static_cast<float>((std::max)(1, frame.framebufferHeight));
        const float panelW = DemoGui::kDemoLauncherPanelWidth * layout.uiScale;
        const float rowStep = layout.ListRowHeight() + layout.ControlGap();
        const float panelH = layout.Padding() * 4.0F + layout.FontLabel() + layout.FormRowHeight() * 4.5F +
                             static_cast<float>(kDemoCount) * rowStep;
        const float panelX = (fbW - panelW) * 0.5F;
        const float panelY = std::max(layout.Padding(), (fbH - panelH) * 0.5F);
        ui.SetNextPanelSize(panelW, panelH);
        ui.SetCursorPos(panelX, panelY);

        if (ui.BeginPanel("launcher", "Spark Demo Launcher")) {
            const Gui::GuiThemePreset preset = Gui::GetActiveGuiThemePreset();
            ui.Text(Gui::GetGuiThemePresetDisplayName(preset));
            ui.SameLine();
            if (ui.Button("theme_prev", "<")) {
                const int n = Gui::GuiThemePresetCount();
                int cur = static_cast<int>(Gui::GetActiveGuiThemePreset());
                cur = ((cur - 1) % n + n) % n;
                OnShellThemeSelected(cur);
            }
            ui.SameLine();
            if (ui.Button("theme_next", ">")) {
                const int n = Gui::GuiThemePresetCount();
                int cur = static_cast<int>(Gui::GetActiveGuiThemePreset());
                cur = ((cur + 1) % n) % n;
                OnShellThemeSelected(cur);
            }
            ui.Separator();
            ui.TextDisabled("Click a demo to launch (keyboard shortcuts still work).");
            for (int i = 0; i < kDemoCount; ++i) {
                char idBuf[24];
                snprintf(idBuf, sizeof(idBuf), "demo_%d", i);
                if (ui.Selectable(idBuf, kDemoLabels[i], launcherSelectedIndex == i)) {
                    launcherSelectedIndex = i;
                    pendingDemoLaunch = i;
                }
            }
            ui.EndPanel();
        }
        Gui::GuiSystem::Get().EndImmediateFrame();
    }

    void SaveShellGuiPreferences() {
        Spark::Gui::SceneEditorLayoutSettings layout{};
        Spark::Gui::TryLoadSceneEditorLayout(layout);
        layout.guiTheme = Spark::Gui::GetActiveGuiThemePreset();
        Spark::Gui::SaveSceneEditorLayout(layout);
    }

    void OnShellThemeSelected(const int idx) {
        Spark::Gui::SetActiveGuiThemePreset(Spark::Gui::GuiThemePresetFromId(idx));
        SaveShellGuiPreferences();
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
        const Spark::Gui::GuiTheme menuSkin =
                Spark::Gui::ResolveGuiTheme(Spark::Gui::GetActiveGuiThemePreset());
        params.ambientColor = {
                menuSkin.shellBackdropBottom.x * 0.14F,
                menuSkin.shellBackdropBottom.y * 0.14F,
                menuSkin.shellBackdropBottom.z * 0.14F};
        params.uiFont = GetWorld().GetUiFont();
        params.uiBoldFont = GetWorld().GetUiBoldFont();
        if (mode == DemoMode::Menu) {
            BuildLauncherPortableUi(context, params);
        }
        context.SetSceneRenderParams(params);
    }

    IEngineContext* engineCtx = nullptr;

    DemoMode mode = DemoMode::Menu;
    int launcherSelectedIndex = -1;
    int pendingDemoLaunch = -1;
    ThreeDDemo threeD{};
    bool threeDLoaded = false;
    SkyDemo skyDemo{};
    bool skyDemoLoaded = false;
    ParticleDemo particleDemo{};
    bool particleDemoLoaded = false;
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
    DemoFpsToggleOverlay fpsOverlay{};
};

UniquePtr<IGame> NewShellDemoGame() {
    return Engine::NewGame<ShellGame>();
}

}  // namespace Spark
