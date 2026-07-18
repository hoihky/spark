#include "spark/demo/MaterialShowcase3DDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"

#include "spark/scene/SceneSubmit.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/Texture2D.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace Spark {
namespace {

Texture2D BuildTangentNormalRipples(const std::uint32_t size, Utf8String name) {
    Texture2D tex(MoveTemp(name));
    Array<std::uint8_t> rgba;
    rgba.Reserve(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U);
    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(size);
            const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(size);
            const float ang = u * TwoPi * 5.0F + v * TwoPi * 3.0F;
            const float nx = std::cos(ang) * 0.55F;
            const float ny = std::sin(ang * 1.25F) * 0.55F;
            const float nz = std::sqrt(std::max(0.0F, 1.0F - nx * nx - ny * ny));
            const auto enc = [](float c) -> std::uint8_t {
                return static_cast<std::uint8_t>(std::clamp((c * 0.5F + 0.5F) * 255.0F, 0.0F, 255.0F));
            };
            rgba.PushBack(enc(nx));
            rgba.PushBack(enc(ny));
            rgba.PushBack(enc(nz));
            rgba.PushBack(255);
        }
    }
    tex.SetPixels(size, size, MoveTemp(rgba));
    return tex;
}

Texture2D BuildEmissiveLayerMap(const std::uint32_t size, Utf8String name) {
    Texture2D tex(MoveTemp(name));
    Array<std::uint8_t> rgba;
    rgba.Reserve(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U);
    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(size);
            const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(size);
            const float wave = 0.5F + 0.5F * std::sin(u * TwoPi * 6.0F + v * TwoPi * 3.0F);
            const float pulse = std::clamp(wave * wave * 1.15F, 0.0F, 1.0F);
            float r = 0.02F;
            float g = 0.02F;
            float b = 0.02F;
            if (u < 1.0F / 3.0F) {
                r = pulse;
                g = pulse * 0.28F;
                b = pulse * 0.08F;
            } else if (u < 2.0F / 3.0F) {
                r = pulse * 0.12F;
                g = pulse * 0.95F;
                b = pulse * 0.32F;
            } else {
                r = pulse * 0.18F;
                g = pulse * 0.42F;
                b = pulse;
            }
            rgba.PushBack(static_cast<std::uint8_t>(std::clamp(r * 255.0F, 0.0F, 255.0F)));
            rgba.PushBack(static_cast<std::uint8_t>(std::clamp(g * 255.0F, 0.0F, 255.0F)));
            rgba.PushBack(static_cast<std::uint8_t>(std::clamp(b * 255.0F, 0.0F, 255.0F)));
            rgba.PushBack(255);
        }
    }
    tex.SetPixels(size, size, MoveTemp(rgba));
    return tex;
}

[[nodiscard]] Vector3 TintForPreset(const int preset) noexcept {
    switch (preset % 4) {
    case 0:
        return {0.82F, 0.62F, 0.38F};
    case 1:
        return {0.55F, 0.72F, 0.92F};
    case 2:
        return {0.92F, 0.42F, 0.38F};
    default:
        return {0.72F, 0.88F, 0.55F};
    }
}

[[nodiscard]] const char* TintPresetName(const int preset) noexcept {
    switch (preset % 4) {
    case 0:
        return "warm";
    case 1:
        return "cool";
    case 2:
        return "coral";
    default:
        return "mint";
    }
}

[[nodiscard]] const char* EmissivePresetName(const int preset) noexcept {
    switch (preset % 4) {
    case 0:
        return "off";
    case 1:
        return "warm HDR";
    case 2:
        return "cyan HDR";
    default:
        return "map × white";
    }
}

void ApplyEmissiveToMaterial(MaterialComponent& mat, const int preset, const bool useEmissiveMap,
        const float intensity, const SharedPtr<Texture2D>& emissiveTex) {
    if (intensity <= 0.0F) {
        mat.SetEmissive(Vector3{}, 0.0F);
        mat.SetEmissiveTexture({});
        return;
    }

    const int activePreset = (preset % 4 == 0) ? 1 : (preset % 4);
    Vector3 color{Vector3::One};
    switch (activePreset) {
    case 1:
        color = {1.0F, 0.42F, 0.12F};
        break;
    case 2:
        color = {0.25F, 0.95F, 1.0F};
        break;
    default:
        break;
    }
    mat.SetEmissive(color, intensity);
    const bool attachMap = useEmissiveMap && activePreset == 3;
    mat.SetEmissiveTexture(attachMap ? emissiveTex : SharedPtr<Texture2D>{});
}

[[nodiscard]] bool EmissiveAdjustDown(IInput& in) noexcept {
    return in.IsKeyPressedThisFrame(GLFW_KEY_Z) || in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET);
}

[[nodiscard]] bool EmissiveAdjustDownHeld(IInput& in) noexcept {
    return in.IsKeyDown(GLFW_KEY_Z) || in.IsKeyDown(GLFW_KEY_LEFT_BRACKET);
}

[[nodiscard]] bool EmissiveAdjustUp(IInput& in) noexcept {
    return in.IsKeyPressedThisFrame(GLFW_KEY_X) || in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET);
}

[[nodiscard]] bool EmissiveAdjustUpHeld(IInput& in) noexcept {
    return in.IsKeyDown(GLFW_KEY_X) || in.IsKeyDown(GLFW_KEY_RIGHT_BRACKET);
}

}  // namespace

void MaterialShowcase3DDemo::ApplyMaterialState() {
    if (showcaseMaterial == nullptr) {
        return;
    }

    showcaseMaterial->SetShadingModel(SceneShadingModel::LitPbr);
    showcaseMaterial->SetBaseColorTexture(useBaseMap ? baseColorTex : SharedPtr<Texture2D>{});
    showcaseMaterial->SetNormalTexture(useNormalMap ? normalTex : SharedPtr<Texture2D>{});
    showcaseMaterial->SetMetallic(metallic);
    showcaseMaterial->SetRoughness(roughness);

    const Vector3 tint = TintForPreset(tintPreset);
    showcaseMaterial->SetTint(tint);
    if (showcaseMesh != nullptr) {
        showcaseMesh->SetAlbedo(tint);
    }

    ApplyEmissiveToMaterial(*showcaseMaterial, emissivePreset, useEmissiveMap, emissiveIntensity, emissiveTex);
}

void MaterialShowcase3DDemo::HandleMaterialInput(IInput& in, const float deltaSeconds) {
    constexpr float kScalarStep = 0.04F;
    constexpr float kScalarHoldRate = 0.55F;

    auto adjustScalar = [&](float& value, const int decKey, const int incKey) {
        if (in.IsKeyPressedThisFrame(decKey)) {
            value = std::clamp(value - kScalarStep, 0.0F, 1.0F);
        } else if (in.IsKeyDown(decKey)) {
            value = std::clamp(value - kScalarHoldRate * deltaSeconds, 0.0F, 1.0F);
        }
        if (in.IsKeyPressedThisFrame(incKey)) {
            value = std::clamp(value + kScalarStep, 0.0F, 1.0F);
        } else if (in.IsKeyDown(incKey)) {
            value = std::clamp(value + kScalarHoldRate * deltaSeconds, 0.0F, 1.0F);
        }
    };

    auto adjustEmissive = [&]() {
        constexpr float kStep = 0.35F;
        constexpr float kHoldRate = 4.5F;
        if (EmissiveAdjustDown(in)) {
            emissiveIntensity = std::max(0.0F, emissiveIntensity - kStep);
        } else if (EmissiveAdjustDownHeld(in)) {
            emissiveIntensity = std::max(0.0F, emissiveIntensity - kHoldRate * deltaSeconds);
        }
        if (EmissiveAdjustUp(in)) {
            if (emissiveIntensity <= 0.0F && emissivePreset % 4 == 0) {
                emissivePreset = 1;
            }
            emissiveIntensity += kStep;
        } else if (EmissiveAdjustUpHeld(in)) {
            if (emissiveIntensity <= 0.0F && emissivePreset % 4 == 0) {
                emissivePreset = 1;
            }
            emissiveIntensity += kHoldRate * deltaSeconds;
        }
    };

    if (in.IsKeyPressedThisFrame(GLFW_KEY_1)) {
        useBaseMap = !useBaseMap;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_2)) {
        useNormalMap = !useNormalMap;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_3)) {
        useEmissiveMap = !useEmissiveMap;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_4)) {
        emissivePreset = (emissivePreset + 1) % 4;
        if (emissivePreset == 1) {
            emissiveIntensity = 4.5F;
        } else if (emissivePreset == 2) {
            emissiveIntensity = 4.0F;
        } else if (emissivePreset == 3) {
            emissiveIntensity = 3.2F;
        } else {
            emissiveIntensity = 0.0F;
        }
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_5)) {
        tintPreset = (tintPreset + 1) % 4;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_0)) {
        useBaseMap = true;
        useNormalMap = false;
        useEmissiveMap = false;
        emissivePreset = 0;
        tintPreset = 0;
        metallic = 0.22F;
        roughness = 0.48F;
        emissiveIntensity = 0.0F;
    }

    adjustScalar(metallic, GLFW_KEY_Q, GLFW_KEY_E);
    adjustScalar(roughness, GLFW_KEY_R, GLFW_KEY_F);
    adjustEmissive();

    ApplyMaterialState();
}

void MaterialShowcase3DDemo::Load(GameWorld& w, IEngineContext& /*context*/) {
    roots.Clear();
    roots.Reserve(8);

    sphereMesh = MakeShared<Mesh>(Utf8String("MatShowSphere"));
    *sphereMesh = Mesh::CreateSkySphere(0.72F, 18, 32);
    w.RegisterMesh(sphereMesh, "spark/demo/matshow_sphere");

    groundMesh = MakeShared<Mesh>(Utf8String("MatShowGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    w.RegisterMesh(groundMesh, "spark/demo/matshow_ground");

    baseColorTex = MakeShared<Texture2D>(Utf8String("MatShowBase"));
    if (!DemoAssets::TryLoadBrickTexture(*baseColorTex)) {
        *baseColorTex = Texture2D::CreateCheckerboard(128, 16, Vector3{0.92F, 0.35F, 0.28F}, Vector3{0.18F, 0.22F, 0.55F});
    }
    w.RegisterTexture(baseColorTex, "spark/demo/matshow_base");
    normalTex = MakeShared<Texture2D>(Utf8String("MatShowNormal"));
    *normalTex = BuildTangentNormalRipples(128, Utf8String("MatShowNormalData"));
    w.RegisterTexture(normalTex, "spark/demo/matshow_normal");
    emissiveTex = MakeShared<Texture2D>(Utf8String("MatShowEmissive"));
    *emissiveTex = BuildEmissiveLayerMap(128, Utf8String("MatShowEmissiveData"));
    w.RegisterTexture(emissiveTex, "spark/demo/matshow_emissive");

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.38F, 0.4F, 0.42F});
    if (MaterialComponent* m = ground->AddComponent<MaterialComponent>()) {
        m->SetShadingModel(SceneShadingModel::LitPbr);
        m->SetMetallic(0.05F);
        m->SetRoughness(0.88F);
    }
    roots.PushBack(ground);

    showcaseSphere = w.CreateGameObject();
    showcaseSphere->GetName() = Utf8String("MatShowcaseSphere");
    showcaseTransform = showcaseSphere->AddComponent<TransformComponent>();
    if (showcaseTransform != nullptr) {
        showcaseTransform->SetTranslation({0.0F, 0.72F, 0.0F});
    }
    showcaseMesh = showcaseSphere->AddComponent<MeshComponent>(sphereMesh, TintForPreset(tintPreset));
    showcaseMaterial = showcaseSphere->AddComponent<MaterialComponent>();
    ApplyMaterialState();
    roots.PushBack(showcaseSphere);

    GameObject* plGo = w.CreateGameObject();
    plGo->GetName() = Utf8String("RimPoint");
    if (TransformComponent* tr = plGo->AddComponent<TransformComponent>()) {
        tr->SetTranslation({-4.2F, 2.4F, 3.5F});
    }
    plGo->AddComponent<PointLightComponent>(Vector3{0.55F, 0.78F, 1.0F}, 2.8F, 16.0F)->SetCastsShadow(true);
    roots.PushBack(plGo);

    GameObject* spotGo = w.CreateGameObject();
    spotGo->GetName() = Utf8String("KeySpot");
    const Vector3 spotPos{4.8F, 4.2F, 6.5F};
    const Vector3 spotTarget{0.0F, 0.65F, 0.0F};
    if (TransformComponent* tr = spotGo->AddComponent<TransformComponent>()) {
        tr->SetTranslation(spotPos);
        const Vector3 dir = (spotTarget - spotPos).Normalized();
        tr->SetRotation(Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, -1.0F}, dir));
    }
    spotGo->AddComponent<SpotLightComponent>(Vector3{1.0F, 0.94F, 0.82F}, 6.0F, 24.0F, 24.0F, 42.0F)
            ->SetCastsShadow(true);
    roots.PushBack(spotGo);

    camera.position = {0.0F, 2.2F, 5.8F};
    camera.SnapLookAt({0.0F, 0.65F, 0.0F});

    helpHud = w.CreateGameObject();
    helpHud->GetName() = Utf8String("MatShowHelp");
    helpText = helpHud->AddComponent<TextOverlayComponent>();
    helpText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
    DemoHud::Apply(*helpText);
    helpText->SetText(Utf8String("…"));
    roots.PushBack(helpHud);
}

void MaterialShowcase3DDemo::Unload(GameWorld& w) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    showcaseSphere = nullptr;
    showcaseMaterial = nullptr;
    showcaseMesh = nullptr;
    showcaseTransform = nullptr;
    helpHud = nullptr;
    helpText = nullptr;
    sphereMesh.Reset();
    groundMesh.Reset();
    baseColorTex.Reset();
    normalTex.Reset();
    emissiveTex.Reset();
}

void MaterialShowcase3DDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsCursorCaptured()) {
        if (timing.frameIndex > 0) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }

    HandleMaterialInput(in, timing.deltaTimeSeconds);

    spinRadians += timing.deltaTimeSeconds * 0.42F;
    if (showcaseTransform != nullptr) {
        showcaseTransform->SetRotation(Quaternion::FromAxisAngle(Vector3{0.0F, 1.0F, 0.0F}, spinRadians));
    }

    if (helpText != nullptr && showcaseMaterial != nullptr) {
        const std::string msg = std::format(
                "Material ball — edit live on one LitPBR sphere\n"
                "  1 base map {} · 2 normal {} · 3 emissive map {} · 4 emissive preset ({}) · 5 tint ({})\n"
                "  Q/E metallic {:.2f} · R/F roughness {:.2f} · Z/X or [ ] emissive {:.1f} · 0 reset\n"
                "  F1 mouse lock · WASD fly · ESC menu",
                useBaseMap ? "on" : "off",
                useNormalMap ? "on" : "off",
                useEmissiveMap ? "on" : "off",
                EmissivePresetName(emissivePreset),
                TintPresetName(tintPreset),
                metallic,
                roughness,
                emissiveIntensity);
        helpText->SetText(Utf8String(msg.c_str()));
    }
}

void MaterialShowcase3DDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(56.0F), aspect, 0.1F, 220.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);

    Vector3 forward = camera.Forward();
    Vector3 pr = Vector3::Cross(Vector3{0.0F, 1.0F, 0.0F}, forward).Normalized();
    if (pr.LengthSquared() < 1e-8F) {
        pr = Vector3::Cross(Vector3{1.0F, 0.0F, 0.0F}, forward).Normalized();
    }
    const Vector3 pu = Vector3::Cross(forward, pr).Normalized();

    SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.32F, 0.88F, 0.34F}.Normalized(),
            Vector3{1.0F, 0.98F, 0.93F},
            1.15F,
            Vector3{0.09F, 0.1F, 0.13F},
            false,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderOnly);
}

}  // namespace Spark
