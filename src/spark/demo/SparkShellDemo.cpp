#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoUi.hpp"
#include "spark/demo/ThreeDDemo.hpp"
#include "spark/demo/ToonShadingDemo.hpp"
#include "spark/demo/MaterialShowcase3DDemo.hpp"
#include "spark/demo/SkyDemo.hpp"
#include "spark/demo/ParticleDemo.hpp"
#include "spark/demo/TerrainDemo.hpp"
#include "spark/demo/CharacterCameraDemo.hpp"
#include "spark/demo/TwoDDemo.hpp"
#include "spark/demo/Tetris2DDemo.hpp"
#include "spark/demo/Connect3Demo.hpp"
#include "spark/demo/SpaceInvaders2DDemo.hpp"
#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/BroadPhase2DDemo.hpp"
#include "spark/demo/RenderLayers2DDemo.hpp"
#include "spark/demo/Maze3DDemo.hpp"
#include "spark/demo/PhysicsBallThrow3DDemo.hpp"
#include "spark/demo/SteeringShowcase3DDemo.hpp"
#include "spark/demo/SceneEditor3DDemo.hpp"
#include "spark/demo/TimeOfDayDemo.hpp"
#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/render/Window.hpp"

#include <cstdio>

namespace Spark {

namespace Detail {

template<typename T>
void GuiShowcasePushRow(Gui::Widget* host, Array<float>& heights, UniquePtr<T> w, const float h) {
    heights.PushBack(h);
    host->AddChild(Spark::MoveTemp(w));
}

}  // namespace Detail

class ShellGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override {
        engineCtx = &context;
        Spark::Gui::SceneEditorLayoutSettings layout{};
        Spark::Gui::TryLoadSceneEditorLayout(layout);
        MountUiFont(GetWorld());
        BuildLauncherUi();
        context.GetInput().SetCursorCaptured(false);
    }

    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        float contentScaleX = 1.0F;
        float contentScaleY = 1.0F;
        context.GetWindow().GetContentScale(contentScaleX, contentScaleY);
        if (mode == DemoMode::Menu || mode == DemoMode::GuiShowcase) {
            context.GetInput().SetCursorCaptured(false);
        }
        Spark::ProcessGuiCanvasesInput(GetScene(), context.GetInput(), fbW, fbH, contentScaleX, contentScaleY);

        if (mode == DemoMode::Menu && engineCtx != nullptr) {
            Spark::IInput& in = context.GetInput();
            if (in.IsKeyPressedThisFrame(GLFW_KEY_1)) {
                EnterThreeD(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_2)) {
                EnterGuiShowcase(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_3)) {
                EnterSkyDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_4)) {
                EnterParticleDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_5)) {
                EnterTerrainDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_6)) {
                EnterCharacterDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_7)) {
                EnterTwoDDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_8)) {
                EnterPlatformer2DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_9)) {
                EnterBroadPhase2DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_0)) {
                EnterMaze3DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_MINUS)) {
                EnterPhysicsBall3DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_EQUAL)) {
                EnterSceneEditor3DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_SEMICOLON)) {
                EnterSteeringShowcase3DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_T)) {
                EnterTetris2DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_C)) {
                EnterConnect3Demo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_I)) {
                EnterSpaceInvaders2DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Y)) {
                EnterToonShadingDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_B)) {
                EnterMaterialShowcase3DDemo(*engineCtx);
            } else if (in.IsKeyPressedThisFrame(GLFW_KEY_N)) {
                EnterTimeOfDayDemo(*engineCtx);
            }
        }

        if (mode == DemoMode::ThreeD) {
            threeD.Simulate(timing, context);
            if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
                ReturnToMenu(context);
            }
        } else if (mode == DemoMode::GuiShowcase) {
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
        } else if (mode == DemoMode::TwoD) {
            twoDDemo.Simulate(timing, context);
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
        }
        Game::OnUpdate(timing, context);
    }

    void OnRender(IRenderFrame& /*frame*/, IEngineContext& context) override {
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (mode == DemoMode::ThreeD) {
            threeD.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Sky) {
            skyDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Particles) {
            particleDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Terrain) {
            terrainDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Character) {
            characterDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::TwoD) {
            twoDDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Tetris2D) {
            tetris2DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Connect3) {
            connect3Demo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::SpaceInvaders2D) {
            spaceInvaders2DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::RenderLayers2D) {
            renderLayers2DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Platformer2D) {
            platformer2DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::BroadPhase2D) {
            broadPhase2DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::Maze3D) {
            maze3DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::PhysicsBall3D) {
            physicsBall3D.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::SteeringShowcase3D) {
            steeringShowcase3D.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::SceneEditor3D) {
            sceneEditor3D.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::ToonShading) {
            toonShadingDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::MaterialShowcase3D) {
            materialShowcase3DDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        if (mode == DemoMode::TimeOfDay) {
            timeOfDayDemo.Render(GetScene(), GetWorld(), context);
            return;
        }
        RenderUiOnly(context, fbW, fbH);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!threeDLoaded) {
            threeD.Load(GetWorld(), context);
            threeDLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!toonShadingLoaded) {
            toonShadingDemo.Load(GetWorld(), context);
            toonShadingLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!materialShowcaseLoaded) {
            materialShowcase3DDemo.Load(GetWorld(), context);
            materialShowcaseLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::MaterialShowcase3D;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterGuiShowcase(IEngineContext& context) {
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        BuildGuiShowcaseUi();
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::GuiShowcase;
        context.GetInput().SetCursorCaptured(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!skyDemoLoaded) {
            skyDemo.Load(GetWorld(), context);
            skyDemoLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
        if (!timeOfDayDemoLoaded) {
            timeOfDayDemo.Load(GetWorld(), context);
            timeOfDayDemoLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!particleDemoLoaded) {
            particleDemo.Load(GetWorld(), context);
            particleDemoLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!terrainDemoLoaded) {
            terrainDemo.Load(GetWorld(), context);
            terrainDemoLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!characterDemoLoaded) {
            characterDemo.Load(GetWorld(), context);
            characterDemoLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::Character;
        context.GetInput().SetCursorCaptured(true);
    }

    void EnterTwoDDemo(IEngineContext& context) {
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
        if (spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Unload(GetWorld());
            spaceInvaders2DLoaded = false;
        }
        DestroyGuiShowcase();
        if (!twoDLoaded) {
            twoDDemo.Load(GetWorld(), context);
            twoDLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::TwoD;
        context.GetInput().SetCursorCaptured(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
        if (!tetris2DLoaded) {
            tetris2DDemo.Load(GetWorld(), context);
            tetris2DLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
        if (!connect3Loaded) {
            connect3Demo.Load(GetWorld(), context);
            connect3Loaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
        if (!spaceInvaders2DLoaded) {
            spaceInvaders2DDemo.Load(GetWorld(), context);
            spaceInvaders2DLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        platformer2DDemo.Load(GetWorld(), context);
        platformer2DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        broadPhase2DDemo.Load(GetWorld(), context);
        broadPhase2DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        if (!renderLayers2DLoaded) {
            renderLayers2DDemo.Load(GetWorld(), context);
            renderLayers2DLoaded = true;
        }
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::RenderLayers2D;
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        maze3DDemo.Load(GetWorld(), context);
        maze3DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
        physicsBall3D.Load(GetWorld(), context);
        physicsBall3DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        UnloadPhysicsBall3DDemoIfAny();
        UnloadTimeOfDayDemoIfAny();
        steeringShowcase3D.Load(GetWorld(), context);
        steeringShowcase3DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        DestroyGuiShowcase();
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        sceneEditor3D.Load(GetWorld(), context);
        sceneEditor3DLoaded = true;
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(false);
        }
        mode = DemoMode::SceneEditor3D;
        context.GetInput().SetCursorCaptured(false);
    }

    void ReturnToMenu(IEngineContext& context) {
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
        if (twoDLoaded) {
            twoDDemo.Unload(GetWorld());
            twoDLoaded = false;
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
        if (maze3DLoaded) {
            maze3DDemo.Unload(GetWorld());
            maze3DLoaded = false;
        }
        DestroyGuiShowcase();
        if (menuCanvas != nullptr) {
            menuCanvas->SetCanvasEnabled(true);
        }
        if (demoListScroll != nullptr) {
            demoListScroll->ScrollToTop();
        }
        if (demoList != nullptr) {
            demoList->SetSelectedIndex(-1);
        }
        GetScene().SetSpatialPartitionKind(Spark::ScenePartitionKind::None);
        mode = DemoMode::Menu;
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

    void BuildLauncherUi() {
        menuGo = GetWorld().CreateGameObject();
        menuGo->GetName() = Spark::Utf8String("Launcher");
        menuCanvas = menuGo->AddComponent<Spark::GuiCanvasComponent>();
        menuCanvas->SetSortOrder(200);
        const Spark::Gui::GuiTheme skin = Spark::Gui::ResolveGuiTheme(Spark::Gui::GetActiveGuiThemePreset());
        menuCanvas->SetTheme(skin);

        auto backdrop = Spark::MakeUnique<Spark::Gui::Panel>();
        menuBackdrop = backdrop.Get();
        backdrop->SetPadding(0.0F);
        backdrop->SetBackgroundGradient(skin.shellBackdropTop, skin.shellBackdropBottom, skin.shellBackdropAlpha);
        backdrop->SetDropShadowEnabled(false);
        backdrop->SetChromeEnabled(false);

        auto layout = Spark::MakeUnique<LauncherMenuLayout>(880.0F);

        auto themeRow = Spark::MakeUnique<LauncherThemeRow>();
        menuThemeRow = themeRow.Get();
        ShellGame* self = this;
        themeRow->SetOnThemeCycle([self](const int delta) {
            const int n = Spark::Gui::GuiThemePresetCount();
            int cur = static_cast<int>(Spark::Gui::GetActiveGuiThemePreset());
            cur = ((cur + delta) % n + n) % n;
            self->OnShellThemeSelected(cur);
        });
        layout->AddChild(Spark::MoveTemp(themeRow));

        auto scroll = Spark::MakeUnique<Spark::Gui::ScrollPanel>();
        demoListScroll = scroll.Get();
        scroll->SetViewportFillGradient(
                skin.scrollViewportTop, skin.scrollViewportBottom, 1.0F);

        auto lst = Spark::MakeUnique<Spark::Gui::List>();
        demoList = lst.Get();
        demoList->SetRowHeight(72.0F);
        demoList->SetItemFontSize(36.0F);
        demoList->SetItemBold(true);
        demoList->SetOpaqueRows(true);
        demoList->SetVerticalScrollingEnabled(false);
        Spark::Array<Spark::Utf8String> items;
        items.PushBack(Spark::Utf8String("1  — Basic 3D scene"));
        items.PushBack(Spark::Utf8String("2  — GUI controls showcase"));
        items.PushBack(Spark::Utf8String("3  — Sky: box, dome, plane"));
        items.PushBack(Spark::Utf8String("4  — Particles"));
        items.PushBack(Spark::Utf8String("5  — Terrain"));
        items.PushBack(Spark::Utf8String("6  — Character (1st / 3rd person)"));
        items.PushBack(Spark::Utf8String("7  — 2D sprites & tilemap"));
        items.PushBack(Spark::Utf8String("8  — 2D platformer"));
        items.PushBack(Spark::Utf8String("9  — 2D Maze"));
        items.PushBack(Spark::Utf8String("10 — 3D maze"));
        items.PushBack(Spark::Utf8String("11 — 3D physics"));
        items.PushBack(Spark::Utf8String("12 — 3D scene editor"));
        items.PushBack(Spark::Utf8String("13 — Tetris"));
        items.PushBack(Spark::Utf8String("14 — Match-3"));
        items.PushBack(Spark::Utf8String("15 — Space Invaders"));
        items.PushBack(Spark::Utf8String("16 — 3D steering"));
        items.PushBack(Spark::Utf8String("17 — Toon/cel shading"));
        items.PushBack(Spark::Utf8String("18 — Material ball"));
        items.PushBack(Spark::Utf8String("19 — Time of day"));
        items.PushBack(Spark::Utf8String("20 — Farming RPG render layers"));
        demoList->SetItems(Spark::MoveTemp(items));
        demoList->SetOnSelectionChanged([self](int idx) {
            if (self->engineCtx == nullptr) {
                return;
            }
            if (idx == 0) {
                self->EnterThreeD(*self->engineCtx);
            } else if (idx == 1) {
                self->EnterGuiShowcase(*self->engineCtx);
            } else if (idx == 2) {
                self->EnterSkyDemo(*self->engineCtx);
            } else if (idx == 3) {
                self->EnterParticleDemo(*self->engineCtx);
            } else if (idx == 4) {
                self->EnterTerrainDemo(*self->engineCtx);
            } else if (idx == 5) {
                self->EnterCharacterDemo(*self->engineCtx);
            } else if (idx == 6) {
                self->EnterTwoDDemo(*self->engineCtx);
            } else if (idx == 7) {
                self->EnterPlatformer2DDemo(*self->engineCtx);
            } else if (idx == 8) {
                self->EnterBroadPhase2DDemo(*self->engineCtx);
            } else if (idx == 9) {
                self->EnterMaze3DDemo(*self->engineCtx);
            } else if (idx == 10) {
                self->EnterPhysicsBall3DDemo(*self->engineCtx);
            } else if (idx == 11) {
                self->EnterSceneEditor3DDemo(*self->engineCtx);
            } else if (idx == 12) {
                self->EnterTetris2DDemo(*self->engineCtx);
            } else if (idx == 13) {
                self->EnterConnect3Demo(*self->engineCtx);
            } else if (idx == 14) {
                self->EnterSpaceInvaders2DDemo(*self->engineCtx);
            } else if (idx == 15) {
                self->EnterSteeringShowcase3DDemo(*self->engineCtx);
            } else if (idx == 16) {
                self->EnterToonShadingDemo(*self->engineCtx);
            } else if (idx == 17) {
                self->EnterMaterialShowcase3DDemo(*self->engineCtx);
            } else if (idx == 18) {
                self->EnterTimeOfDayDemo(*self->engineCtx);
            } else if (idx == 19) {
                self->EnterRenderLayers2DDemo(*self->engineCtx);
            }
        });
        scroll->AddChild(Spark::MoveTemp(lst));
        scroll->ScrollToTop();
        layout->AddChild(Spark::MoveTemp(scroll));

        backdrop->AddChild(Spark::MoveTemp(layout));
        menuCanvas->SetRoot(Spark::MoveTemp(backdrop));
    }

    void BuildGuiShowcaseUi() {
        guiShowcaseGo = GetWorld().CreateGameObject();
        guiShowcaseGo->GetName() = Spark::Utf8String("GuiShowcase");
        auto* cv = guiShowcaseGo->AddComponent<Spark::GuiCanvasComponent>();
        cv->SetSortOrder(150);
        const Spark::Gui::GuiTheme skin = Spark::Gui::ResolveGuiTheme(Spark::Gui::GetActiveGuiThemePreset());
        cv->SetTheme(skin);

        auto root = Spark::MakeUnique<Spark::Gui::Panel>();
        guiShowcaseBackdrop = root.Get();
        root->SetPadding(0.0F);
        root->SetBackgroundGradient(skin.shellBackdropTop, skin.shellBackdropBottom, skin.shellBackdropAlpha);
        root->SetDropShadowEnabled(false);
        root->SetChromeEnabled(false);

        auto dlg = Spark::MakeUnique<Spark::Gui::Dialog>();
        dlg->SetTitle(Spark::Utf8String("GUI subsystem"));
        dlg->SetTitleFontSize(34.0F);
        dlg->SetBodySize(780.0F, 1560.0F);
        Spark::Gui::Panel* body = dlg->ContentPanel();
        body->SetPadding(16.0F);

        auto form = Spark::MakeUnique<ShowcaseFormLayout>();
        auto stackUp = Spark::MakeUnique<ShowcaseVStackForm>();
        ShowcaseVStackForm* const stack = stackUp.Get();
        stack->SetVerticalGap(12.0F);
        Spark::Array<float> rowH;

        auto intro = Spark::MakeUnique<Spark::Gui::WrappingLabel>();
        intro->SetText(Spark::Utf8String(
                "All controls are listed in the dialog body (no scroll). Classic widgets first, then new controls "
                "(slider, progress, scrollbar, switch, numeric drag, tabs, wrap + stack, AlbumCard). Theme presets "
                "are selectable from the launcher menu (persisted in editor_layout.ini).  ESC returns to the menu."));
        intro->SetFontSize(22.0F);
        intro->SetTextColor(skin.labelPrimary);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(intro), 108.0F);

        auto secClassic = Spark::MakeUnique<Spark::Gui::Label>();
        secClassic->SetText(Spark::Utf8String("Classic controls"));
        secClassic->SetFontSize(24.0F);
        secClassic->SetBold(true);
        secClassic->SetTextColor(skin.labelPrimary);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(secClassic), 32.0F);

        auto listLbl = Spark::MakeUnique<Spark::Gui::Label>();
        listLbl->SetText(Spark::Utf8String("Sample list"));
        listLbl->SetFontSize(22.0F);
        listLbl->SetTextColor(skin.labelMuted);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(listLbl), 28.0F);

        auto innerList = Spark::MakeUnique<Spark::Gui::List>();
        innerList->SetRowHeight(48.0F);
        innerList->SetItemFontSize(26.0F);
        Spark::Array<Spark::Utf8String> rows;
        rows.PushBack(Spark::Utf8String("List row A"));
        rows.PushBack(Spark::Utf8String("List row B"));
        rows.PushBack(Spark::Utf8String("List row C"));
        innerList->SetItems(Spark::MoveTemp(rows));
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(innerList), 156.0F);

        auto ddLbl = Spark::MakeUnique<Spark::Gui::Label>();
        ddLbl->SetText(Spark::Utf8String("Dropdown"));
        ddLbl->SetFontSize(22.0F);
        ddLbl->SetTextColor(skin.labelMuted);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(ddLbl), 28.0F);

        auto dd = Spark::MakeUnique<Spark::Gui::Dropdown>();
        dd->SetRowHeight(40.0F);
        dd->SetHeaderFontSize(22.0F);
        dd->SetListItemFontSize(20.0F);
        dd->SetMaxVisibleRows(4);
        Spark::Array<Spark::Utf8String> ddOpts;
        ddOpts.PushBack(Spark::Utf8String("Option Alpha"));
        ddOpts.PushBack(Spark::Utf8String("Option Beta"));
        ddOpts.PushBack(Spark::Utf8String("Option Gamma"));
        ddOpts.PushBack(Spark::Utf8String("Option Delta"));
        ddOpts.PushBack(Spark::Utf8String("Option Epsilon"));
        ddOpts.PushBack(Spark::Utf8String("Option Zeta"));
        dd->SetOptions(Spark::MoveTemp(ddOpts));
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(dd), 44.0F);

        auto cb = Spark::MakeUnique<Spark::Gui::CheckBox>();
        cb->SetLabel(Spark::Utf8String("Enable option"));
        cb->SetFontSize(26.0F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(cb), 52.0F);

        auto tb = Spark::MakeUnique<Spark::Gui::TextBox>();
        tb->SetPlaceholder(Spark::Utf8String("Type here (letters, digits, space, backspace)"));
        tb->SetFontSize(26.0F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(tb), 58.0F);

        auto secNew = Spark::MakeUnique<Spark::Gui::Label>();
        secNew->SetText(Spark::Utf8String("New controls"));
        secNew->SetFontSize(24.0F);
        secNew->SetBold(true);
        secNew->SetTextColor(skin.labelPrimary);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(secNew), 32.0F);

        auto slider = Spark::MakeUnique<Spark::Gui::Slider>();
        slider->SetRange(0.0F, 100.0F);
        slider->SetValue(35.0F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(slider), 48.0F);

        auto prog = Spark::MakeUnique<Spark::Gui::ProgressBar>();
        prog->SetValue01(0.62F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(prog), 36.0F);

        auto hBar = Spark::MakeUnique<Spark::Gui::ScrollBar>();
        hBar->SetAxis(Spark::Gui::ScrollBarAxis::Horizontal);
        hBar->SetValue01(0.35F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(hBar), 34.0F);

        auto sw = Spark::MakeUnique<Spark::Gui::Switch>();
        sw->SetOn(true);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(sw), 44.0F);

        auto num = Spark::MakeUnique<Spark::Gui::NumericBox>();
        num->SetRange(-50.0F, 50.0F);
        num->SetSensitivity(0.08F);
        num->SetValue(0.0F);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(num), 52.0F);

        auto tabs = Spark::MakeUnique<Spark::Gui::TabControl>();
        tabs->SetTabBarHeight(36.0F);
        auto tabA = Spark::MakeUnique<Spark::Gui::Label>();
        tabA->SetText(Spark::Utf8String("Tab A — WrappingLabel-style content can live in each page."));
        tabA->SetFontSize(20.0F);
        tabA->SetTextColor(skin.labelPrimary);
        auto tabB = Spark::MakeUnique<Spark::Gui::Label>();
        tabB->SetText(Spark::Utf8String("Tab B — switch tabs using the strip above."));
        tabB->SetFontSize(20.0F);
        tabB->SetTextColor(skin.labelPrimary);
        tabs->AddTab(Spark::Utf8String("Tab A"), Spark::MoveTemp(tabA));
        tabs->AddTab(Spark::Utf8String("Tab B"), Spark::MoveTemp(tabB));
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(tabs), 200.0F);

        auto wrap = Spark::MakeUnique<Spark::Gui::WrapPanel>();
        wrap->SetSlotSize(168.0F, 40.0F);
        wrap->SetGaps(8.0F, 8.0F);
        for (int i = 0; i < 4; ++i) {
            char buf[48];
            std::snprintf(buf, sizeof(buf), "Wrap cell %d", i + 1);
            auto cell = Spark::MakeUnique<Spark::Gui::Button>();
            cell->SetLabel(Spark::Utf8String(buf));
            cell->SetFontSize(18.0F);
            wrap->AddChild(Spark::MoveTemp(cell));
        }
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(wrap), 52.0F);

        auto hstack = Spark::MakeUnique<Spark::Gui::StackPanel>();
        hstack->SetOrientation(Spark::Gui::StackOrientation::Horizontal);
        hstack->SetSpacing(10.0F);
        for (int c = 0; c < 3; ++c) {
            char buf[24];
            std::snprintf(buf, sizeof(buf), "Col %d", c + 1);
            auto col = Spark::MakeUnique<Spark::Gui::Label>();
            col->SetText(Spark::Utf8String(buf));
            col->SetFontSize(20.0F);
            col->SetTextColor(skin.labelMuted);
            hstack->AddChild(Spark::MoveTemp(col));
        }
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(hstack), 46.0F);

        auto back = Spark::MakeUnique<Spark::Gui::Button>();
        back->SetLabel(Spark::Utf8String("Back to menu"));
        back->SetFontSize(26.0F);
        ShellGame* self = this;
        back->SetOnClick([self]() {
            if (self->engineCtx != nullptr) {
                self->ReturnToMenu(*self->engineCtx);
            }
        });
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(back), 56.0F);

        auto album = Spark::MakeUnique<Spark::Gui::AlbumCard>();
        album->SetTitle(Spark::Utf8String("AlbumCard"));
        album->SetSubtitle(Spark::Utf8String("Album-art style tile — add textured quads when the UI layer supports it."));
        album->SetAccentRailEnabled(true);
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(album), 216.0F);

        auto presetRow = Spark::MakeUnique<Spark::Gui::StackPanel>();
        presetRow->SetOrientation(Spark::Gui::StackOrientation::Horizontal);
        presetRow->SetSpacing(10.0F);
        auto presetLbl = Spark::MakeUnique<Spark::Gui::Label>();
        presetLbl->SetText(Spark::Utf8String("Preset (dropdown omitted):"));
        presetLbl->SetFontSize(22.0F);
        presetLbl->SetTextColor(skin.labelMuted);
        presetRow->AddChild(Spark::MoveTemp(presetLbl));
        for (const char* tag : {"Low", "Medium", "High"}) {
            auto b = Spark::MakeUnique<Spark::Gui::Button>();
            b->SetLabel(Spark::Utf8String(tag));
            b->SetFontSize(20.0F);
            b->SetOnClick([]() {});
            presetRow->AddChild(Spark::MoveTemp(b));
        }
        Detail::GuiShowcasePushRow(stack, rowH, Spark::MoveTemp(presetRow), 52.0F);

        stack->SetRowHeights(Spark::MoveTemp(rowH));
        form->AddChild(Spark::MoveTemp(stackUp));
        body->AddChild(Spark::MoveTemp(form));
        root->AddChild(Spark::MoveTemp(dlg));
        cv->SetRoot(Spark::MoveTemp(root));
    }

    void DestroyGuiShowcase() {
        if (guiShowcaseGo != nullptr) {
            GetWorld().DestroyGameObject(guiShowcaseGo);
            guiShowcaseGo = nullptr;
            guiShowcaseBackdrop = nullptr;
        }
    }

    void SaveShellGuiPreferences() {
        Spark::Gui::SceneEditorLayoutSettings layout{};
        Spark::Gui::TryLoadSceneEditorLayout(layout);
        layout.guiTheme = Spark::Gui::GetActiveGuiThemePreset();
        Spark::Gui::SaveSceneEditorLayout(layout);
    }

    void ApplyShellTheme() {
        const Spark::Gui::GuiTheme skin = Spark::Gui::ResolveGuiTheme(Spark::Gui::GetActiveGuiThemePreset());
        if (menuCanvas != nullptr) {
            menuCanvas->SetTheme(skin);
        }
        if (menuBackdrop != nullptr) {
            menuBackdrop->SetBackgroundGradient(
                    skin.shellBackdropTop, skin.shellBackdropBottom, skin.shellBackdropAlpha);
        }
        if (guiShowcaseGo != nullptr) {
            if (Spark::GuiCanvasComponent* cv = guiShowcaseGo->GetComponent<Spark::GuiCanvasComponent>()) {
                cv->SetTheme(skin);
            }
        }
        if (guiShowcaseBackdrop != nullptr) {
            guiShowcaseBackdrop->SetBackgroundGradient(
                    skin.shellBackdropTop, skin.shellBackdropBottom, skin.shellBackdropAlpha);
        }
        if (menuThemeRow != nullptr) {
            menuThemeRow->RefreshThemeName();
        }
        if (demoListScroll != nullptr) {
            demoListScroll->SetViewportFillGradient(
                    skin.scrollViewportTop, skin.scrollViewportBottom, 1.0F);
        }
    }

    void OnShellThemeSelected(const int idx) {
        Spark::Gui::SetActiveGuiThemePreset(Spark::Gui::GuiThemePresetFromId(idx));
        ApplyShellTheme();
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
        params.ambientColor = {0.08F, 0.11F, 0.08F};
        params.uiFont = GetWorld().GetUiFont();
        params.uiBoldFont = GetWorld().GetUiBoldFont();
        Spark::PaintGuiCanvases(GetScene(), params, fbW, fbH);
        context.SetSceneRenderParams(params);
    }

    IEngineContext* engineCtx = nullptr;

    DemoMode mode = DemoMode::Menu;
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
    TwoDDemo twoDDemo{};
    bool twoDLoaded = false;
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
    Spark::GameObject* menuGo = nullptr;
    Spark::GuiCanvasComponent* menuCanvas = nullptr;
    Spark::Gui::Panel* menuBackdrop = nullptr;
    Spark::Gui::ScrollPanel* demoListScroll = nullptr;
    Spark::Gui::List* demoList = nullptr;
    LauncherThemeRow* menuThemeRow = nullptr;
    Spark::GameObject* guiShowcaseGo = nullptr;
    Spark::Gui::Panel* guiShowcaseBackdrop = nullptr;
};

UniquePtr<IGame> NewShellDemoGame() {
    return Engine::NewGame<ShellGame>();
}

}  // namespace Spark
