#include "spark/demo/SteeringShowcase3DDemo.hpp"

#include "spark/ai/GameAiSubsystem.hpp"

namespace Spark {

void SteeringShowcase3DDemo::Load(GameWorld& w, IEngineContext& context)
{
        roots.Clear();
        hudText = nullptr;
        primaryVel = Vector3::Zero;
        flockVels.Clear();
        flockVels.Reserve(64);
        obstacleCenters.Clear();
        obstacleRadii.Clear();
        pathPoints.Clear();
        pathIndex = 0;
        mode = SteeringShowcaseKind::Seek;
        wanderBoard = AiBlackboard{};
        orbitPursuer = 0.0F;
        orbitSecondary = 0.0F;
        orbitLeader = 0.0F;
        ecsPatrolPathGo = nullptr;
        ecsPatrolAgentGo = nullptr;

        skyMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SteerSky"));
        *skyMesh = Spark::Mesh::CreateSkySphere(1.0F, 12, 24);
        groundMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SteerGround"));
        *groundMesh = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        sphereMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SteerSphere"));
        *sphereMesh = Spark::Mesh::CreateSkySphere(0.35F, 8, 14);
        cubeMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SteerCube"));
        *cubeMesh = Spark::Mesh::CreateUnitCube();

        GameObject* sky = w.CreateGameObject();
        sky->GetName() = Spark::Utf8String("SteerSky");
        TransformComponent* skyTr = sky->AddComponent<TransformComponent>();
        skyTr->SetUniformScale(90.0F);
        sky->AddComponent<SkyComponent>(Spark::SceneSkyMode::Box)->SetTint({0.38F, 0.55F, 0.88F});
        sky->AddComponent<MeshComponent>(skyMesh, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.4F, 0.55F, 0.85F});
        roots.PushBack(sky);

        GameObject* ground = w.CreateGameObject();
        ground->GetName() = Spark::Utf8String("SteerGround");
        ground->AddComponent<TransformComponent>();
        ground->AddComponent<MeshComponent>(
                groundMesh, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.42F, 0.46F, 0.4F});
        roots.PushBack(ground);

        GameObject* light = w.CreateGameObject();
        light->GetName() = Spark::Utf8String("SteerSun");
        TransformComponent* ltr = light->AddComponent<TransformComponent>();
        ltr->SetTranslation({22.0F, 36.0F, 16.0F});
        PointLightComponent* pl = light->AddComponent<PointLightComponent>();
        pl->SetRange(140.0F);
        pl->SetColor({1.0F, 0.97F, 0.9F});
        pl->SetIntensity(2.2F);
        pl->SetCastsShadow(true);
        roots.PushBack(light);

        auto addSphere = [&](const char* name, const Vector3& color) -> GameObject* {
            GameObject* o = w.CreateGameObject();
            o->GetName() = Spark::Utf8String(name);
            TransformComponent* tr = o->AddComponent<TransformComponent>();
            tr->SetUniformScale(1.0F);
            o->AddComponent<MeshComponent>(sphereMesh, Spark::SceneMeshSlot::Custom, color);
            roots.PushBack(o);
            return o;
        };

        targetGo = addSphere("SteerTarget", {0.92F, 0.35F, 0.88F});
        targetGo->GetComponent<TransformComponent>()->SetTranslation({4.0F, 0.55F, -2.0F});
        MaterialComponent* targetMat = targetGo->AddComponent<MaterialComponent>();
        targetMat->SetTint({0.95F, 0.42F, 0.92F});
        targetMat->SetMetallic(0.96F);
        targetMat->SetRoughness(0.04F);
        targetMat->SetEmissive({1.0F, 0.55F, 0.98F}, 4.2F);

        pursuerGo = addSphere("SteerPursuer", {0.9F, 0.25F, 0.22F});
        secondaryGo = addSphere("SteerSecondary", {0.25F, 0.82F, 0.9F});
        leaderGo = addSphere("SteerLeader", {0.35F, 0.9F, 0.4F});

        primaryGo = addSphere("SteerPrimary", {0.95F, 0.82F, 0.2F});
        TransformComponent* pTr = primaryGo->GetComponent<TransformComponent>();
        pTr->SetTranslation({0.0F, 0.55F, 0.0F});
        pTr->SetUniformScale(1.15F);

        constexpr std::size_t kFlock = 28;
        flockGos.Clear();
        flockGos.Reserve(kFlock);
        for (std::size_t i = 0; i < kFlock; ++i) {
            GameObject* b = w.CreateGameObject();
            b->GetName() = Spark::Utf8String("SteerFlock");
            TransformComponent* tr = b->AddComponent<TransformComponent>();
            const float a = static_cast<float>(i) * 0.42F;
            tr->SetTranslation({std::cos(a) * 10.0F, 0.35F, std::sin(a) * 10.0F});
            tr->SetUniformScale(0.22F);
            const float hue = static_cast<float>(i) / static_cast<float>(kFlock);
            b->AddComponent<MeshComponent>(
                    cubeMesh, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.4F + hue * 0.4F, 0.55F, 0.75F});
            flockGos.PushBack(b);
            roots.PushBack(b);
            flockVels.PushBack(Vector3{std::sin(a), 0.0F, std::cos(a)} * 0.8F);
        }

        obstacleCenters.PushBack(Vector3{-12.0F, 0.9F, -4.0F});
        obstacleRadii.PushBack(1.4F);
        obstacleCenters.PushBack(Vector3{10.0F, 1.1F, 4.0F});
        obstacleRadii.PushBack(1.8F);
        obstacleCenters.PushBack(Vector3{0.0F, 1.0F, -14.0F});
        obstacleRadii.PushBack(1.5F);

        obstacleGos.Clear();
        obstacleGos.Reserve(obstacleCenters.GetSize());
        for (std::size_t i = 0; i < obstacleCenters.GetSize(); ++i) {
            GameObject* box = w.CreateGameObject();
            box->GetName() = Spark::Utf8String("SteerObstacle");
            TransformComponent* tr = box->AddComponent<TransformComponent>();
            const float r = obstacleRadii[i];
            tr->SetTranslation(obstacleCenters[i]);
            tr->SetUniformScale(r);
            box->AddComponent<MeshComponent>(
                    cubeMesh, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.55F, 0.5F, 0.52F});
            roots.PushBack(box);
            obstacleGos.PushBack(box);
        }

        constexpr int kPathN = 9;
        for (int i = 0; i < kPathN; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(kPathN - 1);
            const float ang = t * 6.2831855F;
            pathPoints.PushBack(Vector3{std::cos(ang) * 9.0F, 0.55F, std::sin(ang) * 9.0F});
        }

        GameObject* hud = w.CreateGameObject();
        hud->GetName() = Spark::Utf8String("SteerHud");
        hudText = hud->AddComponent<TextOverlayComponent>();
        hudText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*hudText);
        roots.PushBack(hud);

        ecsPatrolPathGo = w.CreateGameObject();
        ecsPatrolPathGo->GetName() = Spark::Utf8String("SteerEcsPatrolPath");
        TransformComponent* ecsPathTr = ecsPatrolPathGo->AddComponent<TransformComponent>();
        ecsPathTr->SetTranslation({-14.0F, 0.0F, -12.0F});
        PatrolPathComponent* ecsPatrol = ecsPatrolPathGo->AddComponent<PatrolPathComponent>();
        ecsPatrol->SetLooping(true);
        const float leg = 5.0F;
        ecsPatrol->GetWaypoints().PushBack(Vector3::Zero);
        ecsPatrol->GetWaypoints().PushBack({leg, 0.0F, 0.0F});
        ecsPatrol->GetWaypoints().PushBack({leg, 0.0F, leg});
        ecsPatrol->GetWaypoints().PushBack({0.0F, 0.0F, leg});
        roots.PushBack(ecsPatrolPathGo);

        ecsPatrolAgentGo = addSphere("SteerEcsPatrolAgent", {0.35F, 0.85F, 0.95F});
        ecsPatrolAgentGo->GetComponent<TransformComponent>()->SetTranslation({-14.0F, 0.55F, -12.0F});
        ecsPatrolAgentGo->GetComponent<TransformComponent>()->SetUniformScale(0.75F);
        NavMeshAgentComponent* ecsNav = ecsPatrolAgentGo->AddComponent<NavMeshAgentComponent>();
        ecsNav->SetPatrolPathObject(ecsPatrolPathGo);
        AiAgentComponent* ecsAgent = ecsPatrolAgentGo->AddComponent<AiAgentComponent>();
        ecsAgent->SetMaxSpeed(2.8F);
        ecsAgent->SetSteeringPlane(AiSteeringPlane::XzWorld);

        MountUiFont(w);
        context.GetInput().SetCursorCaptured(false);
        camera.position = {0.0F, 7.5F, 18.0F};
        camera.SnapLookAt({0.0F, 0.5F, 0.0F});
        camera.moveSpeed = 10.0F;
        camera.mouseSensitivity = 0.12F;
        ResetSteeringDemoEntitiesToInitial();
        ApplySteeringShowcaseVisibility();
    }

void SteeringShowcase3DDemo::Unload(GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        hudText = nullptr;
        targetGo = nullptr;
        pursuerGo = nullptr;
        secondaryGo = nullptr;
        leaderGo = nullptr;
        primaryGo = nullptr;
        ecsPatrolPathGo = nullptr;
        ecsPatrolAgentGo = nullptr;
        flockGos.Clear();
        flockVels.Clear();
        obstacleCenters.Clear();
        obstacleRadii.Clear();
        obstacleGos.Clear();
        pathPoints.Clear();
    }

void SteeringShowcase3DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world)
{
        const float dt = timing.deltaTimeSeconds;
        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured() && timing.frameIndex > 0U) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, dt);

        if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET)) {
            const auto v = static_cast<std::uint8_t>(mode);
            mode = static_cast<SteeringShowcaseKind>((v + static_cast<std::uint8_t>(SteeringShowcaseKind::Count) - 1U) %
                    static_cast<std::uint8_t>(SteeringShowcaseKind::Count));
            ApplySteeringShowcaseVisibility();
            ResetSteeringDemoEntitiesToInitial();
            DemoPlayProceduralClip(context, DemoSfx::ClipSteeringMode(), 0.92F);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET)) {
            const auto v = static_cast<std::uint8_t>(mode);
            mode = static_cast<SteeringShowcaseKind>((v + 1U) % static_cast<std::uint8_t>(SteeringShowcaseKind::Count));
            ApplySteeringShowcaseVisibility();
            ResetSteeringDemoEntitiesToInitial();
            DemoPlayProceduralClip(context, DemoSfx::ClipSteeringMode(), 0.92F);
        }

        auto pick = [&](const SteeringShowcaseKind k) {
            mode = k;
            ApplySteeringShowcaseVisibility();
            ResetSteeringDemoEntitiesToInitial();
            DemoPlayProceduralClip(context, DemoSfx::ClipSteeringMode(), 0.92F);
        };
        using K = SteeringShowcaseKind;
        if (in.IsKeyPressedThisFrame(GLFW_KEY_1)) {
            pick(K::Seek);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_2)) {
            pick(K::Flee);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_3)) {
            pick(K::Arrive);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_4)) {
            pick(K::Pursuit);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_5)) {
            pick(K::Evade);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_6)) {
            pick(K::Wander);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_7)) {
            pick(K::ObstacleAvoidance);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_8)) {
            pick(K::WallAvoidance);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_9)) {
            pick(K::Interpose);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_0)) {
            pick(K::Hide);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_MINUS)) {
            pick(K::PathFollowing);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_EQUAL)) {
            pick(K::OffsetPursuit);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Q)) {
            pick(K::Separation);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_W)) {
            pick(K::Alignment);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_E)) {
            pick(K::Cohesion);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
            pick(K::Flocking);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_T)) {
            pick(K::CombineSeekObstacle);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_Y)) {
            pick(K::CombineFleeWall);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_U)) {
            pick(K::CombineArriveWander);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_I)) {
            pick(K::CombinePursuitObstacle);
        } else if (in.IsKeyPressedThisFrame(GLFW_KEY_O)) {
            pick(K::CombineFlockingObstacle);
        }

        TransformComponent* tgt = targetGo != nullptr ? targetGo->GetComponent<TransformComponent>() : nullptr;
        if (tgt != nullptr && UsesInteractiveTarget(mode)) {
            Vector3 p = tgt->GetLocalTransform().translation;
            const float sp = 9.0F;
            if (in.IsKeyDown(GLFW_KEY_UP)) {
                p.z -= sp * dt;
            }
            if (in.IsKeyDown(GLFW_KEY_DOWN)) {
                p.z += sp * dt;
            }
            if (in.IsKeyDown(GLFW_KEY_LEFT)) {
                p.x -= sp * dt;
            }
            if (in.IsKeyDown(GLFW_KEY_RIGHT)) {
                p.x += sp * dt;
            }
            p.y = 0.55F;
            tgt->SetTranslation(p);
        }

        orbitPursuer += dt * 0.55F;
        orbitSecondary += dt * 0.38F;
        orbitLeader += dt * 0.42F;
        if (pursuerGo != nullptr) {
            TransformComponent* tr = pursuerGo->GetComponent<TransformComponent>();
            tr->SetTranslation({std::cos(orbitPursuer) * 11.0F, 0.55F, std::sin(orbitPursuer) * 11.0F});
        }
        if (secondaryGo != nullptr) {
            TransformComponent* tr = secondaryGo->GetComponent<TransformComponent>();
            tr->SetTranslation({std::sin(orbitSecondary) * 7.0F, 0.55F, std::cos(orbitSecondary * 1.3F) * 7.0F});
        }
        if (leaderGo != nullptr) {
            TransformComponent* tr = leaderGo->GetComponent<TransformComponent>();
            tr->SetTranslation({std::cos(orbitLeader) * 8.5F, 0.55F, std::sin(orbitLeader) * 8.5F - 3.0F});
        }

        SteeringEnvironment3D env{};
        env.worldBoundsMin = {-36.0F, 0.2F, -36.0F};
        env.worldBoundsMax = {36.0F, 20.0F, 36.0F};
        env.obstacleCenters = &obstacleCenters;
        env.obstacleRadii = &obstacleRadii;
        env.pathPoints = &pathPoints;
        env.pathIndex = pathIndex;
        env.pathWaypointRadius = 1.1F;
        env.pathLoop = true;
        if (tgt != nullptr) {
            env.targetPosition = tgt->GetLocalTransform().translation;
        }
        env.targetVelocity = Vector3{std::sin(orbitSecondary * 0.7F), 0.0F, std::cos(orbitSecondary * 0.7F)} * 3.0F;
        if (secondaryGo != nullptr) {
            env.secondaryPosition = secondaryGo->GetComponent<TransformComponent>()->GetLocalTransform().translation;
        }
        env.secondaryVelocity = Vector3{std::cos(orbitSecondary), 0.0F, -std::sin(orbitSecondary)} * 2.0F;
        if (pursuerGo != nullptr) {
            env.pursuerPosition = pursuerGo->GetComponent<TransformComponent>()->GetLocalTransform().translation;
        }
        env.pursuerVelocity = Vector3{-std::sin(orbitPursuer) * 6.0F, 0.0F, std::cos(orbitPursuer) * 6.0F};
        if (leaderGo != nullptr) {
            env.leaderPosition = leaderGo->GetComponent<TransformComponent>()->GetLocalTransform().translation;
        }
        env.leaderVelocity = Vector3{-std::sin(orbitLeader) * 5.0F, 0.0F, std::cos(orbitLeader) * 5.0F};

        Array<Vector3> flockPos;
        flockPos.Reserve(flockGos.GetSize());
        for (std::size_t i = 0; i < flockGos.GetSize(); ++i) {
            TransformComponent* tr = flockGos[i]->GetComponent<TransformComponent>();
            flockPos.PushBack(tr->GetLocalTransform().translation);
        }

        const bool isFlockMode = (mode == SteeringShowcaseKind::Separation || mode == SteeringShowcaseKind::Alignment ||
                                  mode == SteeringShowcaseKind::Cohesion || mode == SteeringShowcaseKind::Flocking ||
                                  mode == SteeringShowcaseKind::CombineFlockingObstacle);
        env.maxSteeringSpeed = isFlockMode ? 7.0F : 9.0F;
        env.maxAcceleration = 24.0F;
        if (isFlockMode) {
            for (std::size_t fi = 0; fi < flockGos.GetSize(); ++fi) {
                env.flockPositions = &flockPos;
                env.flockVelocities = &flockVels;
                env.flockSelfIndex = fi;
                const Vector3 acc =
                        ComputeSteeringForMode(mode, flockPos[fi], flockVels[fi], env, wanderBoard);
                flockVels[fi] += acc * dt;
                flockVels[fi] = ClampHorizSpeed(flockVels[fi], 7.0F);
                Vector3 np = flockPos[fi] + flockVels[fi] * dt;
                np.y = 0.35F;
                flockGos[fi]->GetComponent<TransformComponent>()->SetTranslation(np);
            }
        }

        if (!isFlockMode && primaryGo != nullptr) {
            TransformComponent* pTr = primaryGo->GetComponent<TransformComponent>();
            Vector3 pos = pTr->GetLocalTransform().translation;
            Array<Vector3> neighborPos = flockPos;
            Array<Vector3> neighborVel = flockVels;
            neighborPos.PushBack(pos);
            neighborVel.PushBack(primaryVel);
            env.flockSelfIndex = neighborPos.GetSize() - 1U;
            env.flockPositions = &neighborPos;
            env.flockVelocities = &neighborVel;
            env.pathIndex = pathIndex;

            const Vector3 acc = ComputeSteeringForMode(mode, pos, primaryVel, env, wanderBoard);
            primaryVel += acc * dt;
            primaryVel = ClampHorizSpeed(primaryVel, 9.0F);
            pos += primaryVel * dt;
            pos.y = 0.55F;
            pTr->SetTranslation(pos);
            if (mode == SteeringShowcaseKind::PathFollowing && pathPoints.GetSize() > 0) {
                const Vector3 wp = pathPoints[static_cast<std::size_t>(pathIndex)];
                if ((pos - wp).LengthSquared() < env.pathWaypointRadius * env.pathWaypointRadius) {
                    ++pathIndex;
                    if (pathIndex >= static_cast<int>(pathPoints.GetSize())) {
                        pathIndex = env.pathLoop ? 0 : static_cast<int>(pathPoints.GetSize()) - 1;
                    }
                }
            }
        }

        ResolveSteeringDemoCollisions();
        SimulateGameAi(world, timing, context);

        if (hudText != nullptr) {
            hudText->SetText(Spark::Utf8String(
                    std::format(
                            "[ ] cycle · 1-9 Seek…Interpose · 0 Hide · - Path · = Offset · QWER flock · TYUI O "
                            "combines · arrows move magenta target · cyan agent = ECS AiAgent patrol · F1 · ESC\n{}",
                            ModeName(mode))
                            .c_str()));
        }
    }

void SteeringShowcase3DDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
        const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 320.0F);
        const Matrix4 view = camera.ViewMatrix();
        const Matrix4 viewProj = proj * view;

        SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Vector3{0.32F, 0.86F, 0.35F}.Normalized();
        params.lightColor = {1.0F, 0.96F, 0.92F};
        params.lightIntensity = 0.95F;
        params.ambientColor = {0.10F, 0.12F, 0.16F};
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
        params.draws.Reserve(64);

        Array<SceneDrawItem> drawList;
        drawList.Reserve(64);
        scene.ForEachPointLight([&params](const PointLightComponent& pl, const Matrix4& worldMat) {
            if (params.pointLights.GetSize() >= SceneRenderParams::MaxPointLights) {
                return;
            }
            ScenePointLight gpu{};
            gpu.positionWorld = worldMat.TranslationVector();
            gpu.range = pl.GetRange();
            gpu.color = pl.GetColor();
            gpu.intensity = pl.GetIntensity();
            gpu.castsShadow = pl.CastsShadow();
            params.pointLights.PushBack(gpu);
        });

        scene.ForEachSky([&](GameObject&, const SkyComponent& sk, const MeshComponent& mc, const MaterialComponent* mat,
                                const Matrix4& world) {
            SceneDrawItem item{};
            item.mesh = SceneMeshSlot::Custom;
            item.skyMode = sk.GetSkyMode();
            item.model = world;
            item.customMesh = mc.GetMesh();
            item.albedo = sk.GetTint();
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 1.0F;
            if (mat != nullptr && mat->GetBaseColorTexture()) {
                const Vector3& t = mat->GetTint();
                item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
            }
            drawList.PushBack(item);
        });

        scene.ForEachDrawable([&](GameObject* obj, const MeshComponent& mc, const MaterialComponent* mat,
                                     const Matrix4& world) {
            if (obj != nullptr && obj->GetComponent<SkyComponent>() != nullptr) {
                return;
            }
            SceneDrawItem item{};
            item.model = world;
            item.mesh = mc.GetSlot();
            if (mc.GetSlot() == SceneMeshSlot::Custom) {
                item.customMesh = mc.GetMesh();
            }
            Vector3 alb = mc.GetAlbedo();
            item.textureLayer = -1;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Vector3& t = mat->GetTint();
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

        scene.ForEachTextOverlay([&params](const TextOverlayComponent& tc) {
            ScreenTextDraw d{};
            d.text = tc.GetText();
            d.x = tc.GetScreenX();
            d.y = tc.GetScreenY();
            d.sizePixels = tc.GetFontSizePixels();
            d.color = tc.GetColor();
            d.alpha = tc.GetAlpha();
            d.paintOrder = params.NextUiPaintOrder();
            params.screenTexts.PushBack(MoveTemp(d));
        });

        PaintGuiCanvases(world, params, fbW, fbH);
        context.SetSceneRenderParams(params);
    }

[[nodiscard]] bool SteeringShowcase3DDemo::DrawableCollisionMesh(const GameObject* o) noexcept
{
        if (o == nullptr) {
            return false;
        }
        const MeshComponent* mc = o->GetComponent<MeshComponent>();
        return mc != nullptr && mc->GetMesh().Get() != nullptr;
    }

/** Kinematic overlap resolution: unit cubes vs spheres (mesh radii match CreateSkySphere / CreateUnitCube). */
    void SteeringShowcase3DDemo::ResolveSteeringDemoCollisions() noexcept
{
        constexpr float kSphereMeshRadius = 0.35F;
        constexpr float kUnitCubeHalf = 1.0F;
        constexpr float kSlop = 0.015F;
        constexpr int kPasses = 7;

        struct SphereBody {
            TransformComponent* tr = nullptr;
            float radius = 0.35F;
            float lockY = 0.55F;
        };
        struct BoxBody {
            TransformComponent* tr = nullptr;
            Vector3 half{1.0F, 1.0F, 1.0F};
            bool immovable = false;
            float lockY = 0.55F;
        };

        Array<SphereBody> spheres;
        Array<BoxBody> boxes;
        spheres.Reserve(8);
        boxes.Reserve(64);

        auto addSphere = [&](GameObject* go, const float lockY) {
            if (!DrawableCollisionMesh(go)) {
                return;
            }
            TransformComponent* tr = go->GetComponent<TransformComponent>();
            const float s = tr->GetLocalTransform().scale.x;
            spheres.PushBack(SphereBody{tr, kSphereMeshRadius * s, lockY});
        };
        auto addBox = [&](GameObject* go, const bool immovable, const float lockY) {
            if (!DrawableCollisionMesh(go)) {
                return;
            }
            TransformComponent* tr = go->GetComponent<TransformComponent>();
            const Vector3& sc = tr->GetLocalTransform().scale;
            boxes.PushBack(BoxBody{tr, {sc.x * kUnitCubeHalf, sc.y * kUnitCubeHalf, sc.z * kUnitCubeHalf}, immovable,
                            lockY});
        };

        addSphere(targetGo, 0.55F);
        addSphere(pursuerGo, 0.55F);
        addSphere(secondaryGo, 0.55F);
        addSphere(leaderGo, 0.55F);
        addSphere(primaryGo, 0.55F);
        for (std::size_t i = 0; i < flockGos.GetSize(); ++i) {
            addBox(flockGos[i], false, 0.35F);
        }
        for (std::size_t i = 0; i < obstacleGos.GetSize(); ++i) {
            addBox(obstacleGos[i], true, obstacleCenters[i].y);
        }

        auto aabbFrom = [](const Vector3& c, const Vector3& h) noexcept {
            CollisionAabb3 a{};
            a.minX = c.x - h.x;
            a.maxX = c.x + h.x;
            a.minY = c.y - h.y;
            a.maxY = c.y + h.y;
            a.minZ = c.z - h.z;
            a.maxZ = c.z + h.z;
            return a;
        };

        auto readPos = [](const TransformComponent* tr) noexcept { return tr->GetLocalTransform().translation; };

        auto writePos = [](TransformComponent* tr, Vector3 p, const float lockY) noexcept {
            p.y = lockY;
            tr->SetTranslation(p);
        };

        auto resolveSphereSphere = [&](SphereBody& a, SphereBody& b) noexcept {
            Vector3 pa = readPos(a.tr);
            Vector3 pb = readPos(b.tr);
            const Vector3 d = pb - pa;
            const float minD = a.radius + b.radius;
            const float d2 = d.LengthSquared();
            if (d2 >= minD * minD - 1.0e-8F) {
                return;
            }
            const float dist = std::sqrt((std::max)(d2, 1.0e-12F));
            const float pen = (minD - dist) + kSlop;
            Vector3 n = dist > 1.0e-5F ? d * (1.0F / dist) : Vector3{1.0F, 0.0F, 0.0F};
            pa -= n * (pen * 0.5F);
            pb += n * (pen * 0.5F);
            writePos(a.tr, pa, a.lockY);
            writePos(b.tr, pb, b.lockY);
        };

        auto resolveSphereBox = [&](SphereBody& s, BoxBody& b, const float wSphere, const float wBox) noexcept {
            if (wSphere <= 0.0F && wBox <= 0.0F) {
                return;
            }
            Vector3 c = readPos(s.tr);
            const Vector3 bc = readPos(b.tr);
            const CollisionAabb3 box = aabbFrom(bc, b.half);
            float qx = c.x;
            if (qx < box.minX) {
                qx = box.minX;
            } else if (qx > box.maxX) {
                qx = box.maxX;
            }
            float qy = c.y;
            if (qy < box.minY) {
                qy = box.minY;
            } else if (qy > box.maxY) {
                qy = box.maxY;
            }
            float qz = c.z;
            if (qz < box.minZ) {
                qz = box.minZ;
            } else if (qz > box.maxZ) {
                qz = box.maxZ;
            }
            const Vector3 q{qx, qy, qz};
            Vector3 delta = c - q;
            float dist = delta.Length();
            Vector3 n{};
            float pen = 0.0F;
            if (dist > 1.0e-5F) {
                if (dist >= s.radius - 1.0e-5F) {
                    return;
                }
                n = delta * (1.0F / dist);
                pen = (s.radius - dist) + kSlop;
            } else {
                const float dxl = c.x - box.minX;
                const float dxr = box.maxX - c.x;
                const float dyl = c.y - box.minY;
                const float dyr = box.maxY - c.y;
                const float dzl = c.z - box.minZ;
                const float dzr = box.maxZ - c.z;
                float best = dxl;
                n = Vector3{-1.0F, 0.0F, 0.0F};
                if (dxr < best) {
                    best = dxr;
                    n = {1.0F, 0.0F, 0.0F};
                }
                if (dyl < best) {
                    best = dyl;
                    n = {0.0F, -1.0F, 0.0F};
                }
                if (dyr < best) {
                    best = dyr;
                    n = {0.0F, 1.0F, 0.0F};
                }
                if (dzl < best) {
                    best = dzl;
                    n = {0.0F, 0.0F, -1.0F};
                }
                if (dzr < best) {
                    best = dzr;
                    n = {0.0F, 0.0F, 1.0F};
                }
                pen = (s.radius - best) + kSlop;
                if (pen <= 0.0F) {
                    return;
                }
            }
            const float wSum = wSphere + wBox;
            if (wSum <= 1.0e-8F) {
                return;
            }
            c += n * (pen * (wSphere / wSum));
            Vector3 bc2 = bc;
            bc2 -= n * (pen * (wBox / wSum));
            writePos(s.tr, c, s.lockY);
            writePos(b.tr, bc2, b.lockY);
        };

        auto resolveBoxBox = [&](BoxBody& a, BoxBody& b) noexcept {
            if (a.immovable && b.immovable) {
                return;
            }
            Vector3 ca = readPos(a.tr);
            Vector3 cb = readPos(b.tr);
            const Vector3 d = cb - ca;
            const float px = a.half.x + b.half.x - std::fabs(d.x);
            const float py = a.half.y + b.half.y - std::fabs(d.y);
            const float pz = a.half.z + b.half.z - std::fabs(d.z);
            if (px <= 0.0F || py <= 0.0F || pz <= 0.0F) {
                return;
            }
            float pen = px;
            Vector3 n{d.x >= 0.0F ? 1.0F : -1.0F, 0.0F, 0.0F};
            if (py < pen) {
                pen = py;
                n = {0.0F, d.y >= 0.0F ? 1.0F : -1.0F, 0.0F};
            }
            if (pz < pen) {
                pen = pz;
                n = {0.0F, 0.0F, d.z >= 0.0F ? 1.0F : -1.0F};
            }
            pen += kSlop;
            const float wA = a.immovable ? 0.0F : 1.0F;
            const float wB = b.immovable ? 0.0F : 1.0F;
            const float wSum = wA + wB;
            if (wSum <= 1.0e-8F) {
                return;
            }
            ca -= n * (pen * (wA / wSum));
            cb += n * (pen * (wB / wSum));
            writePos(a.tr, ca, a.lockY);
            writePos(b.tr, cb, b.lockY);
        };

        for (int pass = 0; pass < kPasses; ++pass) {
            for (std::size_t i = 0; i < spheres.GetSize(); ++i) {
                for (std::size_t j = i + 1U; j < spheres.GetSize(); ++j) {
                    resolveSphereSphere(spheres[i], spheres[j]);
                }
            }
            for (std::size_t si = 0; si < spheres.GetSize(); ++si) {
                for (std::size_t bi = 0; bi < boxes.GetSize(); ++bi) {
                    const float wB = boxes[bi].immovable ? 0.0F : 0.5F;
                    const float wS = boxes[bi].immovable ? 1.0F : 0.5F;
                    resolveSphereBox(spheres[si], boxes[bi], wS, wB);
                }
            }
            for (std::size_t i = 0; i < boxes.GetSize(); ++i) {
                for (std::size_t j = i + 1U; j < boxes.GetSize(); ++j) {
                    resolveBoxBox(boxes[i], boxes[j]);
                }
            }
        }
    }

/** Restores actors to the same layout as a fresh load (orbit timers zero, velocities cleared). */
    void SteeringShowcase3DDemo::ResetSteeringDemoEntitiesToInitial() noexcept
{
        orbitPursuer = 0.0F;
        orbitSecondary = 0.0F;
        orbitLeader = 0.0F;
        pathIndex = 0;
        primaryVel = Vector3::Zero;
        wanderBoard = AiBlackboard{};

        if (targetGo != nullptr) {
            targetGo->GetComponent<TransformComponent>()->SetTranslation({4.0F, 0.55F, -2.0F});
        }
        if (pursuerGo != nullptr) {
            pursuerGo->GetComponent<TransformComponent>()->SetTranslation(
                    {std::cos(orbitPursuer) * 11.0F, 0.55F, std::sin(orbitPursuer) * 11.0F});
        }
        if (secondaryGo != nullptr) {
            secondaryGo->GetComponent<TransformComponent>()->SetTranslation(
                    {std::sin(orbitSecondary) * 7.0F, 0.55F, std::cos(orbitSecondary * 1.3F) * 7.0F});
        }
        if (leaderGo != nullptr) {
            leaderGo->GetComponent<TransformComponent>()->SetTranslation(
                    {std::cos(orbitLeader) * 8.5F, 0.55F, std::sin(orbitLeader) * 8.5F - 3.0F});
        }
        if (primaryGo != nullptr) {
            TransformComponent* pTr = primaryGo->GetComponent<TransformComponent>();
            pTr->SetTranslation({0.0F, 0.55F, 0.0F});
            pTr->SetUniformScale(1.15F);
        }
        for (std::size_t i = 0; i < flockGos.GetSize(); ++i) {
            const float a = static_cast<float>(i) * 0.42F;
            flockGos[i]->GetComponent<TransformComponent>()->SetTranslation(
                    {std::cos(a) * 10.0F, 0.35F, std::sin(a) * 10.0F});
            if (i < flockVels.GetSize()) {
                flockVels[i] = Vector3{std::sin(a), 0.0F, std::cos(a)} * 0.8F;
            }
        }
    }

void SteeringShowcase3DDemo::SetActorMesh(GameObject* go, const SharedPtr<Mesh>& mesh, const bool visible) noexcept
{
        if (go == nullptr) {
            return;
        }
        MeshComponent* mc = go->GetComponent<MeshComponent>();
        if (mc == nullptr) {
            return;
        }
        if (visible) {
            mc->SetMesh(mesh);
        } else {
            mc->SetMesh(SharedPtr<Mesh>{});
        }
    }

void SteeringShowcase3DDemo::SetFlockMeshesVisible(const bool visible) noexcept
{
        for (std::size_t i = 0; i < flockGos.GetSize(); ++i) {
            SetActorMesh(flockGos[i], cubeMesh, visible);
        }
    }

void SteeringShowcase3DDemo::SetObstacleMeshesVisible(const bool visible) noexcept
{
        for (std::size_t i = 0; i < obstacleGos.GetSize(); ++i) {
            SetActorMesh(obstacleGos[i], cubeMesh, visible);
        }
    }

/** Modes where the magenta target is visible and should respond to arrow keys. */
[[nodiscard]] bool SteeringShowcase3DDemo::UsesInteractiveTarget(const SteeringShowcaseKind m) noexcept
{
        using S = SteeringShowcaseKind;
        switch (m) {
        case S::Seek:
        case S::Flee:
        case S::Arrive:
        case S::Pursuit:
        case S::Interpose:
        case S::CombineSeekObstacle:
        case S::CombineFleeWall:
        case S::CombineArriveWander:
        case S::CombinePursuitObstacle:
        case S::Hide:
            return true;
        default:
            return false;
        }
    }

void SteeringShowcase3DDemo::ApplySteeringShowcaseVisibility() noexcept
{
        using S = SteeringShowcaseKind;
        const S m = mode;

        bool showTarget = false;
        bool showPursuer = false;
        bool showSecondary = false;
        bool showLeader = false;
        bool showPrimary = true;
        bool showFlock = false;
        bool showObstacles = false;

        switch (m) {
        case S::Seek:
        case S::Flee:
        case S::Arrive:
            showTarget = true;
            break;
        case S::Pursuit:
            showTarget = true;
            showSecondary = true;
            break;
        case S::Evade:
            showPursuer = true;
            break;
        case S::Wander:
        case S::WallAvoidance:
        case S::PathFollowing:
            break;
        case S::ObstacleAvoidance:
            showObstacles = true;
            break;
        case S::Interpose:
            showTarget = true;
            showSecondary = true;
            break;
        case S::Hide:
            showTarget = true;
            showPursuer = true;
            showObstacles = true;
            break;
        case S::OffsetPursuit:
            showLeader = true;
            break;
        case S::Separation:
        case S::Alignment:
        case S::Cohesion:
        case S::Flocking:
            showPrimary = false;
            showFlock = true;
            break;
        case S::CombineSeekObstacle:
            showTarget = true;
            showObstacles = true;
            break;
        case S::CombineFleeWall:
        case S::CombineArriveWander:
            showTarget = true;
            break;
        case S::CombinePursuitObstacle:
            showTarget = true;
            showSecondary = true;
            showObstacles = true;
            break;
        case S::CombineFlockingObstacle:
            showPrimary = false;
            showFlock = true;
            showObstacles = true;
            break;
        case S::Count:
            break;
        }

        SetActorMesh(targetGo, sphereMesh, showTarget);
        SetActorMesh(pursuerGo, sphereMesh, showPursuer);
        SetActorMesh(secondaryGo, sphereMesh, showSecondary);
        SetActorMesh(leaderGo, sphereMesh, showLeader);
        SetActorMesh(primaryGo, sphereMesh, showPrimary);
        SetFlockMeshesVisible(showFlock);
        SetObstacleMeshesVisible(showObstacles);
    }

[[nodiscard]] const char* SteeringShowcase3DDemo::ModeName(const SteeringShowcaseKind m) noexcept
{
        switch (m) {
        case SteeringShowcaseKind::Seek:
            return "Seek";
        case SteeringShowcaseKind::Flee:
            return "Flee";
        case SteeringShowcaseKind::Arrive:
            return "Arrive";
        case SteeringShowcaseKind::Pursuit:
            return "Pursuit";
        case SteeringShowcaseKind::Evade:
            return "Evade";
        case SteeringShowcaseKind::Wander:
            return "Wander";
        case SteeringShowcaseKind::ObstacleAvoidance:
            return "Obstacle avoidance";
        case SteeringShowcaseKind::WallAvoidance:
            return "Wall avoidance";
        case SteeringShowcaseKind::Interpose:
            return "Interpose";
        case SteeringShowcaseKind::Hide:
            return "Hide";
        case SteeringShowcaseKind::PathFollowing:
            return "Path following";
        case SteeringShowcaseKind::OffsetPursuit:
            return "Offset pursuit";
        case SteeringShowcaseKind::Separation:
            return "Separation (flock)";
        case SteeringShowcaseKind::Alignment:
            return "Alignment (flock)";
        case SteeringShowcaseKind::Cohesion:
            return "Cohesion (flock)";
        case SteeringShowcaseKind::Flocking:
            return "Flocking (sep+align+coh)";
        case SteeringShowcaseKind::CombineSeekObstacle:
            return "Combine: Seek + obstacle";
        case SteeringShowcaseKind::CombineFleeWall:
            return "Combine: Flee + wall";
        case SteeringShowcaseKind::CombineArriveWander:
            return "Combine: Arrive + wander";
        case SteeringShowcaseKind::CombinePursuitObstacle:
            return "Combine: Pursuit + obstacle";
        case SteeringShowcaseKind::CombineFlockingObstacle:
            return "Combine: Flocking + obstacle";
        default:
            return "?";
        }
    }

[[nodiscard]] Vector3 SteeringShowcase3DDemo::ClampHorizSpeed(const Vector3& v, const float maxSp) noexcept
{
        const Vector3 h{v.x, 0.0F, v.z};
        const float m2 = h.LengthSquared();
        if (m2 <= maxSp * maxSp) {
            return v;
        }
        const Vector3 hn = h.Normalized() * maxSp;
        return {hn.x, v.y, hn.z};
    }

[[nodiscard]] Vector3 SteeringShowcase3DDemo::ComputeSteeringForMode(
            const SteeringShowcaseKind mode,
            const Vector3& pos,
            const Vector3& vel,
            const SteeringEnvironment3D& env,
            AiBlackboard& board)
{
        SteeringSeek3D seek(1.0F);
        SteeringFlee3D flee(1.0F);
        SteeringArrive3D arrive(1.0F);
        SteeringPursuit3D pursuit(1.0F);
        SteeringEvade3D evade(1.0F);
        SteeringWander3D wander(0.85F);
        SteeringObstacleAvoidance3D obs(1.0F);
        SteeringWallAvoidance3D wall(1.1F);
        SteeringInterpose3D inter(1.0F);
        SteeringHide3D hide(1.0F);
        SteeringPathFollowing3D path(1.0F);
        SteeringOffsetPursuit3D off(1.0F);
        SteeringSeparation3D sep(1.0F);
        SteeringAlignment3D ali(0.9F);
        SteeringCohesion3D coh(0.75F);
        SteeringFlocking3D flock(1.0F, 1.2F, 0.85F, 0.65F);

        switch (mode) {
        case SteeringShowcaseKind::Seek:
            return seek.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Flee:
            return flee.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Arrive:
            return arrive.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Pursuit:
            return pursuit.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Evade:
            return evade.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Wander:
            return wander.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::ObstacleAvoidance:
            return obs.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::WallAvoidance:
            return wall.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Interpose:
            return inter.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Hide:
            return hide.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::PathFollowing:
            return path.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::OffsetPursuit:
            return off.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Separation:
            return sep.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Alignment:
            return ali.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Cohesion:
            return coh.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::Flocking:
            return flock.Compute(pos, vel, env, board);
        case SteeringShowcaseKind::CombineSeekObstacle: {
            SteeringComposer3D c;
            c.AddBehavior(seek, 0.55F);
            c.AddBehavior(obs, 1.0F);
            return c.Compose(pos, vel, env, board);
        }
        case SteeringShowcaseKind::CombineFleeWall: {
            SteeringComposer3D c;
            c.AddBehavior(flee, 0.7F);
            c.AddBehavior(wall, 1.0F);
            return c.Compose(pos, vel, env, board);
        }
        case SteeringShowcaseKind::CombineArriveWander: {
            SteeringComposer3D c;
            c.AddBehavior(arrive, 0.85F);
            c.AddBehavior(wander, 0.35F);
            return c.Compose(pos, vel, env, board);
        }
        case SteeringShowcaseKind::CombinePursuitObstacle: {
            SteeringComposer3D c;
            c.AddBehavior(pursuit, 0.75F);
            c.AddBehavior(obs, 1.0F);
            return c.Compose(pos, vel, env, board);
        }
        case SteeringShowcaseKind::CombineFlockingObstacle: {
            SteeringComposer3D c;
            c.AddBehavior(flock, 1.0F);
            c.AddBehavior(obs, 0.85F);
            return c.Compose(pos, vel, env, board);
        }
        default:
            return Vector3::Zero;
        }
    }
}  // namespace Spark
