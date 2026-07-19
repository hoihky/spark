#include "spark/demo/platformer2d/Platformer2DHealthHud.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Spark::Platformer2D {

namespace {

Spark::SharedPtr<Spark::Texture2D> MakeWhitePixelTexture()
{
    Spark::Texture2D tex(Spark::Utf8String("PlatHudWhitePixel"));
    Spark::Array<std::uint8_t> px;
    px.Resize(4U);
    px[0] = 255;
    px[1] = 255;
    px[2] = 255;
    px[3] = 255;
    tex.SetPixels(1U, 1U, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

}  // namespace

void HealthHud::Initialize(
        Spark::GameWorld& world,
        const Spark::SharedPtr<Spark::Texture2D>& whitePixelTex,
        Spark::DemoRootCollection& roots)
{
    Shutdown(world);
    Spark::SharedPtr<Spark::Texture2D> tex = whitePixelTex;
    if (tex.Get() == nullptr) {
        tex = MakeWhitePixelTexture();
    }

    root = world.CreateGameObject();
    root->GetName() = Spark::Utf8String("PlatHealthHud");
    rootTr = root->AddComponent<Spark::TransformComponent>();
    roots.Track(root);

    Spark::GameObject* trackGo = world.CreateGameObject();
    trackGo->GetName() = Spark::Utf8String("PlatHealthTrack");
    trackTr = trackGo->AddComponent<Spark::TransformComponent>();
    trackGo->AddComponent<Spark::SpriteComponent>(
            tex,
            Spark::Vector4{0.12F, 0.05F, 0.06F, 0.88F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            9800);
    roots.Track(trackGo);

    Spark::GameObject* fillGo = world.CreateGameObject();
    fillGo->GetName() = Spark::Utf8String("PlatHealthFill");
    fillTr = fillGo->AddComponent<Spark::TransformComponent>();
    fillSpr = fillGo->AddComponent<Spark::SpriteComponent>(
            tex,
            Spark::Vector4{0.88F, 0.14F, 0.16F, 0.96F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            9801);
    roots.Track(fillGo);

    Spark::GameObject* glossGo = world.CreateGameObject();
    glossGo->GetName() = Spark::Utf8String("PlatHealthGloss");
    glossTr = glossGo->AddComponent<Spark::TransformComponent>();
    glossGo->AddComponent<Spark::SpriteComponent>(
            tex,
            Spark::Vector4{1.0F, 0.45F, 0.48F, 0.35F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            9802);
    roots.Track(glossGo);

    Spark::GameObject* textGo = world.CreateGameObject();
    textGo->GetName() = Spark::Utf8String("PlatHealthText");
    healthText = textGo->AddComponent<Spark::TextOverlayComponent>();
    healthText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
    healthText->SetFontSizePixels(22.0F);
    Spark::DemoHud::Apply(*healthText, true);
    roots.Track(textGo);

    displayedRatio = 1.0F;
    targetRatio = 1.0F;
}

void HealthHud::Shutdown(Spark::GameWorld& /*world*/) noexcept
{
    root = nullptr;
    rootTr = nullptr;
    trackTr = nullptr;
    fillTr = nullptr;
    glossTr = nullptr;
    fillSpr = nullptr;
    healthText = nullptr;
    currentHealth = 0.0F;
    maxHealth = 1.0F;
}

void HealthHud::SyncToCamera(
        const Spark::Camera2DComponent& camera,
        const Spark::GameObject& cameraOwner,
        const float framebufferWidth,
        const float framebufferHeight) noexcept
{
    if (rootTr == nullptr || trackTr == nullptr || fillTr == nullptr || glossTr == nullptr) {
        return;
    }
    const Spark::Camera2D cam = camera.BuildCamera2D(cameraOwner);
    const float halfX = cam.halfExtentY * (framebufferHeight > 1.0e-3F ? (framebufferWidth / framebufferHeight) : 1.0F);
    const float leftX = cam.position.x - halfX;
    const float topY = cam.position.y + cam.halfExtentY;
    const float marginX = 0.55F;
    const float marginY = 0.55F;
    const float textRowWorld =
            framebufferHeight > 1.0e-3F ? (34.0F / framebufferHeight) * (2.0F * cam.halfExtentY) : 0.45F;

    barWidthWorld = std::min(halfX * 1.55F, 13.5F);
    barHeightWorld = 0.42F;
    const float barLeft = leftX + marginX;
    const float barCenterX = barLeft + barWidthWorld * 0.5F;
    const float barY = topY - marginY - textRowWorld;

    displayedRatio += (targetRatio - displayedRatio) * 0.14F;
    const float fillWidth = std::max(0.08F, barWidthWorld * displayedRatio);

    rootTr->SetTranslation({barCenterX, barY, 0.2F});

    trackTr->SetTranslation({barCenterX, barY, 0.201F});
    trackTr->SetScale({barWidthWorld, barHeightWorld, 1.0F});

    const float fillLeft = barLeft + fillWidth * 0.5F;
    fillTr->SetTranslation({fillLeft, barY, 0.202F});
    fillTr->SetScale({fillWidth, barHeightWorld * 0.82F, 1.0F});

    glossTr->SetTranslation({fillLeft, barY + barHeightWorld * 0.14F, 0.203F});
    glossTr->SetScale({fillWidth * 0.98F, barHeightWorld * 0.22F, 1.0F});

    if (fillSpr != nullptr) {
        const float ratio = std::clamp(targetRatio, 0.0F, 1.0F);
        const Spark::Vector3 low{0.92F, 0.18F, 0.16F};
        const Spark::Vector3 mid{0.95F, 0.55F, 0.12F};
        const Spark::Vector3 high{0.28F, 0.82F, 0.34F};
        Spark::Vector3 rgb = low;
        if (ratio > 0.55F) {
            rgb = high;
        } else if (ratio > 0.28F) {
            rgb = mid;
        }
        fillSpr->SetTint({rgb.x, rgb.y, rgb.z, 0.96F});
    }

    if (healthText != nullptr) {
        healthText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        char label[48];
        std::snprintf(
                label,
                sizeof(label),
                "HP %.0f / %.0f",
                static_cast<double>(currentHealth),
                static_cast<double>(maxHealth));
        healthText->SetText(Spark::Utf8String(label));
    }
}

void HealthHud::SetHealth(const float current, const float maximum) noexcept
{
    currentHealth = current;
    maxHealth = maximum;
    if (maximum <= 1.0e-4F) {
        targetRatio = 0.0F;
        return;
    }
    targetRatio = std::clamp(current / maximum, 0.0F, 1.0F);
}

}  // namespace Spark::Platformer2D
