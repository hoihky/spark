#include "spark/demo/PhysicsBallThrow3DDemo.hpp"

#include "spark/gui/GuiThemeCatalog.hpp"

namespace Spark {

void PhysicsBallThrow3DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        ClearGuiRefs();
        cubeObjects.Clear();
        cubeRubber.Clear();
        ballRb = nullptr;
        ballTr = nullptr;
        pendulumBobRb = nullptr;
        fpsText = nullptr;
        guiCanvasGo = nullptr;

        skyMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("PhysBallSky"));
        *skyMesh = Spark::Mesh::CreateSkySphere(1.0F, 16, 32);
        groundMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("PhysBallGround"));
        *groundMesh = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        cubeMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("PhysBallCube"));
        *cubeMesh = Spark::Mesh::CreateUnitCube();
        ballMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("PhysBallSphere"));
        /** ~Regulation basketball radius (~0.12 m) in world units. */
        constexpr float kBallRadius = 0.12F;
        *ballMesh = Spark::Mesh::CreateSkySphere(kBallRadius, 10, 20);

        Spark::GameObject* sky = w.CreateGameObject();
        sky->GetName() = Spark::Utf8String("PhysBallSky");
        Spark::TransformComponent* skyTr = sky->AddComponent<Spark::TransformComponent>();
        skyTr->SetUniformScale(96.0F);
        Spark::SkyComponent* skyComp = sky->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Box);
        skyComp->SetTint({0.42F, 0.58F, 0.92F});
        sky->AddComponent<Spark::MeshComponent>(skyMesh, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.35F, 0.48F, 0.72F});
        roots.PushBack(sky);

        Spark::GameObject* ground = w.CreateGameObject();
        ground->GetName() = Spark::Utf8String("PhysBallGround");
        ground->AddComponent<Spark::TransformComponent>();
        ground->AddComponent<Spark::MeshComponent>(
                groundMesh, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.45F, 0.5F, 0.42F});
        {
            const float hx = Spark::kSceneGroundHalfExtent;
            const float hy = 0.18F;
            ground->AddComponent<Spark::BoxCollider3DComponent>(Spark::Vector3{hx, hy, hx}, Spark::Vector3{0.0F, hy, 0.0F});
            ground->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Static, 1.0F);
            /** Restitution third arg: solver uses <c>e = rb * √(e_ball·e_surface)</c>; keep floor in sport-court range. */
            ground->AddComponent<Spark::PhysicsMaterial3DComponent>(0.62F, 0.48F, 0.58F);
        }
        roots.PushBack(ground);

        struct CubeDef {
            float x, z;
        };
        static constexpr CubeDef kCubes[] = {
                {-6.0F, 4.0F},
                {2.5F, -5.0F},
                {-3.0F, -6.5F},
                {7.0F, 2.0F},
                {0.0F, 8.0F},
                {-8.0F, -3.0F},
        };
        std::size_t cubeIndex = 0;
        for (const CubeDef& c : kCubes) {
            Spark::GameObject* box = w.CreateGameObject();
            box->GetName() = Spark::Utf8String("PhysBallBox");
            Spark::TransformComponent* tr = box->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({c.x, 1.0F, c.z});
            tr->SetUniformScale(1.0F);
            box->AddComponent<Spark::MeshComponent>(
                    cubeMesh, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.72F, 0.58F, 0.42F});
            box->AddComponent<Spark::BoxCollider3DComponent>(Spark::Vector3{1.0F, 1.0F, 1.0F}, Spark::Vector3::Zero);
            box->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Static, 1.0F);
            const bool rubber = (cubeIndex % 3U == 1U);
            if (rubber) {
                box->AddComponent<Spark::PhysicsMaterial3DComponent>(0.38F, 0.28F, 0.88F);
            } else {
                box->AddComponent<Spark::PhysicsMaterial3DComponent>(0.58F, 0.42F, 0.34F);
            }
            cubeObjects.PushBack(box);
            cubeRubber.PushBack(rubber);
            roots.PushBack(box);
            ++cubeIndex;
        }

        Spark::GameObject* ball = w.CreateGameObject();
        ball->GetName() = Spark::Utf8String("PhysBall");
        ballTr = ball->AddComponent<Spark::TransformComponent>();
        ballTr->SetTranslation({0.0F, 1.25F, 2.0F});
        ball->AddComponent<Spark::MeshComponent>(ballMesh, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.92F, 0.35F, 0.18F});
        ball->AddComponent<Spark::SphereCollider3DComponent>(kBallRadius, Spark::Vector3::Zero);
        ballRb = ball->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Dynamic, 1.0F);
        /** Use 1 so pair restitution is <c>√(e_ball_mat · e_surface)</c> (see <c>PairRestitution</c> in PhysicsWorld3D). */
        ballRb->SetRestitution(1.0F);
        ballRb->SetInverseMass(1.0F / 0.62F);
        ballRb->SetLinearDamping(0.004F);
        ballRb->SetAngularDamping(0.06F);
        ballRb->SetVelocity(Spark::Vector3::Zero);
        ballRb->SetAngularVelocity(Spark::Vector3::Zero);
        ball->AddComponent<Spark::PhysicsMaterial3DComponent>(0.48F, 0.38F, 0.86F);
        roots.PushBack(ball);

        {
            constexpr float kPendRadius = 0.11F;
            constexpr float kPendRestLength = 2.1F;
            Spark::GameObject* anchor = w.CreateGameObject();
            anchor->GetName() = Spark::Utf8String("PhysBallPendulumAnchor");
            Spark::TransformComponent* anchorTr = anchor->AddComponent<Spark::TransformComponent>();
            anchorTr->SetTranslation({-5.5F, 4.8F, -2.5F});
            anchor->AddComponent<Spark::MeshComponent>(
                    cubeMesh, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.35F, 0.38F, 0.42F});
            anchor->AddComponent<Spark::SphereCollider3DComponent>(kPendRadius, Spark::Vector3::Zero);
            anchor->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Static, 1.0F);
            roots.PushBack(anchor);

            Spark::GameObject* bob = w.CreateGameObject();
            bob->GetName() = Spark::Utf8String("PhysBallPendulumBob");
            Spark::TransformComponent* bobTr = bob->AddComponent<Spark::TransformComponent>();
            bobTr->SetTranslation({-5.5F, 4.8F - kPendRestLength, -2.5F});
            bob->AddComponent<Spark::MeshComponent>(
                    ballMesh, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.88F, 0.55F, 0.2F});
            bob->AddComponent<Spark::SphereCollider3DComponent>(kPendRadius, Spark::Vector3::Zero);
            pendulumBobRb = bob->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Dynamic, 1.0F);
            pendulumBobRb->SetInverseMass(1.0F / 0.35F);
            pendulumBobRb->SetLinearDamping(0.01F);
            Spark::SpringJoint3DComponent* spring = bob->AddComponent<Spark::SpringJoint3DComponent>(anchor, kPendRestLength);
            spring->SetSpringStiffness(38.0F);
            spring->SetDamping(3.8F);
            roots.PushBack(bob);
        }

        Spark::GameObject* light = w.CreateGameObject();
        light->GetName() = Spark::Utf8String("PhysBallSun");
        Spark::TransformComponent* ltr = light->AddComponent<Spark::TransformComponent>();
        ltr->SetTranslation({18.0F, 28.0F, 14.0F});
        Spark::PointLightComponent* pl = light->AddComponent<Spark::PointLightComponent>();
        pl->SetRange(120.0F);
        pl->SetColor({1.0F, 0.97F, 0.88F});
        pl->SetIntensity(2.4F);
        pl->SetCastsShadow(true);
        roots.PushBack(light);

        Spark::GameObject* hud = w.CreateGameObject();
        hud->GetName() = Spark::Utf8String("PhysBallHud");
        fpsText = hud->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        fpsText->SetText(Spark::Utf8String(
                "Physics — panel right · LMB throw · R reset · SpringJoint3D pendulum left · WASD+mouse · F1 · ESC"));
        roots.PushBack(hud);

        guiCanvasGo = w.CreateGameObject();
        guiCanvasGo->GetName() = Spark::Utf8String("PhysBallGui");
        Spark::GuiCanvasComponent* guiCv = guiCanvasGo->AddComponent<Spark::GuiCanvasComponent>();
        guiCv->SetSortOrder(260);
        BuildPhysTuningPanel(*guiCv);
        roots.PushBack(guiCanvasGo);

        ApplyTuningFromGui();

        camera.position = {0.0F, 2.1F, 10.0F};
        camera.SnapLookAt({0.0F, 1.0F, 0.0F});
        camera.moveSpeed = 5.5F;
        camera.mouseSensitivity = 0.12F;

        MountUiFont(w);
        context.GetInput().SetCursorCaptured(false);
    }

void PhysicsBallThrow3DDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        ClearGuiRefs();
        cubeObjects.Clear();
        cubeRubber.Clear();
        ballRb = nullptr;
        ballTr = nullptr;
        pendulumBobRb = nullptr;
        fpsText = nullptr;
        guiCanvasGo = nullptr;
    }

void PhysicsBallThrow3DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        const float dt = timing.deltaTimeSeconds;
        ApplyTuningFromGui();
        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured() && timing.frameIndex > 0U) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, dt);

        if (ballRb != nullptr && ballTr != nullptr) {
            if (in.IsMouseButtonPressedThisFrame(0)) {
                const Spark::Vector3 dir = camera.Forward().Normalized();
                const Spark::Vector3 spawn = camera.position + dir * 0.55F;
                ballTr->SetTranslation(spawn);
                /** Small upward component (m/s) for a natural arc; scales slightly with throw speed. */
                const float upKick = 0.35F + throwSpeed * 0.06F;
                ballRb->SetAngularVelocity(Spark::Vector3::Zero);
                ballRb->SetVelocity(dir * throwSpeed + Spark::Vector3::UnitY * upKick);
                DemoPlayProceduralClip(context, DemoSfx::ClipPhysicsThrow(), 1.0F);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
                ballTr->SetTranslation({0.0F, 1.25F, 2.0F});
                ballRb->SetVelocity(Spark::Vector3::Zero);
                ballRb->SetAngularVelocity(Spark::Vector3::Zero);
                DemoPlayProceduralClip(context, DemoSfx::ClipPhysicsThrow(), 0.55F);
            }
        }

        Spark::Vector3 ballVelBeforeStep = Spark::Vector3::Zero;
        if (ballRb != nullptr) {
            ballVelBeforeStep = ballRb->GetVelocity();
        }

        Spark::PhysicsWorld3DSettings phys{};
        phys.gravityY = gravityY;
        phys.maxFallSpeed = 130.0F;
        phys.resolveIterations = 10;
        phys.substeps = 3;
        phys.jointIterations = 8;
        /** Impulses are first-iter only in the solver; Baumgarte here fights clean bouncing. */
        phys.baumgarteContactBias = 0.0F;
        Spark::SimulatePhysics3D(world, timing, phys);

        if (ballRb != nullptr) {
            const Spark::Vector3 va = ballRb->GetVelocity();
            const float prevY = ballVelBeforeStep.y;
            const float deltaY = va.y - prevY;
            if (prevY < -2.2F && deltaY > 2.8F) {
                DemoPlayProceduralClip(context, DemoSfx::ClipPhysicsBounce(), 0.88F);
            }
        }

        if (fpsText != nullptr) {
            const float tdt = timing.deltaTimeSeconds;
            const float instant = (tdt > 1.0e-6F) ? (1.0F / tdt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            Spark::Vector3 v{};
            if (ballRb != nullptr) {
                v = ballRb->GetVelocity();
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format("Ball v ({:.1f},{:.1f},{:.1f}) m/s  {:.0f} FPS",
                                  static_cast<double>(v.x),
                                  static_cast<double>(v.y),
                                  static_cast<double>(v.z),
                                  static_cast<double>(fpsSmoothed))
                            .c_str()));
        }
    }

void PhysicsBallThrow3DDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.1F, 320.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.35F, 0.88F, 0.32F}.Normalized();
        params.lightColor = {1.0F, 0.96F, 0.9F};
        params.lightIntensity = 0.95F;
        params.ambientColor = {0.09F, 0.11F, 0.14F};

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(24);

        scene.ForEachPointLight([&params](const Spark::PointLightComponent& pl, const Spark::Matrix4& worldMat) {
            if (params.pointLights.GetSize() >= Spark::SceneRenderParams::MaxPointLights) {
                return;
            }
            Spark::ScenePointLight gpu{};
            gpu.positionWorld = worldMat.TranslationVector();
            gpu.range = pl.GetRange();
            gpu.color = pl.GetColor();
            gpu.intensity = pl.GetIntensity();
            gpu.castsShadow = pl.CastsShadow();
            params.pointLights.PushBack(gpu);
        });

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(24);
        scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                                  const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            Spark::SceneDrawItem item{};
            item.mesh = Spark::SceneMeshSlot::Custom;
            item.skyMode = sk.GetSkyMode();
            item.model = world;
            item.customMesh = mc.GetMesh();
            item.albedo = sk.GetTint();
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 1.0F;
            if (mat != nullptr && mat->GetBaseColorTexture()) {
                const Spark::Vector3& t = mat->GetTint();
                item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
            }
            drawList.PushBack(item);
        });

        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc, const Spark::MaterialComponent* mat,
                                     const Spark::Matrix4& world) {
            if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
                return;
            }
            Spark::SceneDrawItem item{};
            item.model = world;
            item.mesh = mc.GetSlot();
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                item.customMesh = mc.GetMesh();
            }
            Spark::Vector3 alb = mc.GetAlbedo();
            item.textureLayer = -1;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    alb = {alb.x * t.x, alb.y * t.y, alb.z * t.z};
                }
            }
            item.albedo = alb;
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        for (std::size_t di = 0; di < drawList.GetSize(); ++di) {
            params.draws.PushBack(drawList[di]);
        }

        scene.ForEachTextOverlay([&params](const Spark::TextOverlayComponent& tc) {
            Spark::ScreenTextDraw d{};
            d.text = tc.GetText();
            d.x = tc.GetScreenX();
            d.y = tc.GetScreenY();
            d.sizePixels = tc.GetFontSizePixels();
            d.color = tc.GetColor();
            d.alpha = tc.GetAlpha();
            d.paintOrder = params.NextUiPaintOrder();
            params.screenTexts.PushBack(Spark::MoveTemp(d));
        });

        Spark::PaintGuiCanvases(world, params, fbW, fbH);
        context.SetSceneRenderParams(params);
    }

void PhysicsBallThrow3DDemo::ClearGuiRefs() noexcept
{
        guiGravity = nullptr;
        guiBallMass = nullptr;
        guiThrow = nullptr;
        guiCubeBounce = nullptr;
        guiCubeMass = nullptr;
    }

void PhysicsBallThrow3DDemo::BuildPhysTuningPanel(Spark::GuiCanvasComponent& canvas)
{
        ClearGuiRefs();

        const Spark::Gui::GuiTheme skin =
                Spark::Gui::ResolveGuiTheme(Spark::Gui::GetActiveGuiThemePreset());
        canvas.SetTheme(skin);

        auto root = Spark::MakeUnique<PhysicsBallGuiDockRoot>();
        auto shell = Spark::MakeUnique<Spark::Gui::Panel>();
        shell->SetPadding(16.0F);
        shell->SetChromeEnabled(true);
        shell->SetDropShadowEnabled(true);
        shell->SetBackgroundGradient(skin.panelElevatedTop, skin.panelElevatedBottom, skin.panelElevatedAlpha);

        auto stack = Spark::MakeUnique<Spark::Gui::StackPanel>();
        stack->SetOrientation(Spark::Gui::StackOrientation::Vertical);
        stack->SetSpacing(8.0F);

        auto title = Spark::MakeUnique<Spark::Gui::Label>();
        title->SetText(Spark::Utf8String("Physics tuning"));
        title->SetFontSize(26.0F);
        title->SetBold(true);
        stack->AddChild(Spark::MoveTemp(title));

        auto help = Spark::MakeUnique<Spark::Gui::WrappingLabel>();
        help->SetText(Spark::Utf8String(
                "F1 toggles mouse capture. Sliders use SI-style units (m/s², kg). Bounce uses ball + surface "
                "materials (geometric mean) in the solver; cube bounciness retunes dull vs rubber crates."));
        help->SetFontSize(17.0F);
        help->SetTone(Spark::Gui::LabelTone::Muted);
        stack->AddChild(Spark::MoveTemp(help));

        PhysicsBallThrow3DDemo* self = this;
        auto addSliderRow = [&stack](const char* title, const float r0, const float r1, Spark::Gui::Slider*& outPtr) {
            auto row = Spark::MakeUnique<Spark::Gui::StackPanel>();
            row->SetOrientation(Spark::Gui::StackOrientation::Horizontal);
            row->SetSpacing(10.0F);
            auto lab = Spark::MakeUnique<Spark::Gui::Label>();
            lab->SetText(Spark::Utf8String(title));
            lab->SetFontSize(18.0F);
            lab->SetTone(Spark::Gui::LabelTone::Muted);
            auto sl = Spark::MakeUnique<Spark::Gui::Slider>();
            outPtr = sl.Get();
            sl->SetRange(r0, r1);
            row->AddChild(Spark::MoveTemp(lab));
            row->AddChild(Spark::MoveTemp(sl));
            stack->AddChild(Spark::MoveTemp(row));
        };

        addSliderRow("Gravity Y (m/s²)", -18.0F, -4.0F, guiGravity);
        addSliderRow("Ball mass (kg)", 0.2F, 8.0F, guiBallMass);
        addSliderRow("Throw speed (m/s)", 3.0F, 28.0F, guiThrow);
        addSliderRow("Cube bounciness", 0.0F, 1.0F, guiCubeBounce);
        addSliderRow("Cube mass (kg)", 40.0F, 220.0F, guiCubeMass);

        if (guiGravity != nullptr) {
            guiGravity->SetValue(-9.81F);
        }
        if (guiBallMass != nullptr) {
            guiBallMass->SetValue(0.62F);
        }
        if (guiThrow != nullptr) {
            guiThrow->SetValue(12.0F);
        }
        if (guiCubeBounce != nullptr) {
            guiCubeBounce->SetValue(0.48F);
        }
        if (guiCubeMass != nullptr) {
            guiCubeMass->SetValue(95.0F);
        }

        if (guiGravity != nullptr) {
            guiGravity->SetOnChanged([self](const float v) {
                self->gravityY = v;
            });
        }
        if (guiBallMass != nullptr) {
            guiBallMass->SetOnChanged([self](const float v) {
                if (self->ballRb != nullptr) {
                    self->ballRb->SetInverseMass(1.0F / (std::max)(0.08F, v));
                }
            });
        }
        if (guiThrow != nullptr) {
            guiThrow->SetOnChanged([self](const float v) { self->throwSpeed = v; });
        }
        if (guiCubeBounce != nullptr) {
            guiCubeBounce->SetOnChanged([self](const float v) { self->ApplyCubeBounciness(v); });
        }
        if (guiCubeMass != nullptr) {
            guiCubeMass->SetOnChanged([self](const float v) { self->ApplyCubeMassScale(v); });
        }

        shell->AddChild(Spark::MoveTemp(stack));
        root->AddChild(Spark::MoveTemp(shell));
        canvas.SetRoot(Spark::MoveTemp(root));
    }

void PhysicsBallThrow3DDemo::ApplyTuningFromGui() noexcept
{
        if (guiGravity != nullptr) {
            gravityY = guiGravity->GetValue();
        }
        if (guiThrow != nullptr) {
            throwSpeed = guiThrow->GetValue();
        }
        if (ballRb != nullptr && guiBallMass != nullptr) {
            ballRb->SetInverseMass(1.0F / (std::max)(0.08F, guiBallMass->GetValue()));
        }
        if (guiCubeBounce != nullptr) {
            ApplyCubeBounciness(guiCubeBounce->GetValue());
        }
        if (guiCubeMass != nullptr) {
            ApplyCubeMassScale(guiCubeMass->GetValue());
        }
    }

void PhysicsBallThrow3DDemo::ApplyCubeBounciness(const float tIn) noexcept
{
        const float t = std::clamp(tIn, 0.0F, 1.0F);
        for (std::size_t i = 0; i < cubeObjects.GetSize(); ++i) {
            Spark::GameObject* go = cubeObjects[i];
            if (go == nullptr) {
                continue;
            }
            auto* mat = go->GetComponent<Spark::PhysicsMaterial3DComponent>();
            if (mat == nullptr) {
                continue;
            }
            const bool rubber = (i < cubeRubber.GetSize()) && cubeRubber[i];
            if (rubber) {
                mat->SetRestitution(std::clamp(0.52F + t * 0.44F, 0.0F, 1.0F));
            } else {
                mat->SetRestitution(std::clamp(0.22F + t * 0.46F, 0.0F, 1.0F));
            }
        }
    }

void PhysicsBallThrow3DDemo::ApplyCubeMassScale(const float massKg) noexcept
{
        constexpr float kRefKg = 95.0F;
        const float ratio = (std::max)(15.0F, massKg) / kRefKg;
        const float s = std::clamp(std::cbrt(ratio), 0.86F, 1.18F);
        for (std::size_t i = 0; i < cubeObjects.GetSize(); ++i) {
            if (Spark::GameObject* go = cubeObjects[i]) {
                if (Spark::TransformComponent* tr = go->GetComponent<Spark::TransformComponent>()) {
                    tr->SetUniformScale(s);
                }
            }
        }
    }
}  // namespace Spark
