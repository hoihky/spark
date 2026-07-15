#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiControls.hpp"
#include "spark/gui/GuiPaintContext.hpp"

namespace Spark {

namespace Detail {

constexpr int kParticleDemoEffectCount = 4;

inline void ApplyParticlePreset(const int presetIndex, ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    switch (presetIndex) {
    case 0:  // Fire
        pe.SetMaxParticles(800);
        pe.SetEmissionRate(110.0F);
        pe.SetLifetime(0.22F, 0.55F);
        pe.SetStartEndSize(0.24F, 0.03F);
        pe.SetStartEndColor(
                Vector4{1.0F, 0.55F, 0.12F, 1.0F}, Vector4{0.85F, 0.05F, 0.0F, 0.0F});
        pe.SetGravity({0.0F, 0.35F, 0.0F});
        pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
        pe.SetSpreadAngleRadians(0.55F);
        pe.SetSpeedRange(1.6F, 4.2F);
        break;
    case 1:  // Snow
        pe.SetMaxParticles(1400);
        pe.SetEmissionRate(240.0F);
        pe.SetLifetime(2.0F, 3.8F);
        pe.SetStartEndSize(0.07F, 0.035F);
        pe.SetStartEndColor(
                Vector4{0.95F, 0.97F, 1.0F, 0.95F}, Vector4{0.88F, 0.92F, 1.0F, 0.0F});
        pe.SetGravity({0.0F, -0.55F, 0.0F});
        pe.SetEmissionDirection(Spark::Vector3{0.05F, -1.0F, 0.02F}.Normalized());
        pe.SetSpreadAngleRadians(1.25F);
        pe.SetSpeedRange(0.15F, 1.1F);
        break;
    case 2:  // Smoke
        pe.SetMaxParticles(500);
        pe.SetEmissionRate(38.0F);
        pe.SetLifetime(1.4F, 2.6F);
        pe.SetStartEndSize(0.1F, 0.42F);
        pe.SetStartEndColor(
                Vector4{0.55F, 0.55F, 0.55F, 0.55F}, Vector4{0.35F, 0.35F, 0.35F, 0.0F});
        pe.SetGravity({0.0F, 0.85F, 0.0F});
        pe.SetEmissionDirection(Spark::Vector3{0.12F, 1.0F, 0.08F}.Normalized());
        pe.SetSpreadAngleRadians(0.95F);
        pe.SetSpeedRange(0.25F, 1.05F);
        break;
    default:  // Magic
        pe.SetMaxParticles(900);
        pe.SetEmissionRate(85.0F);
        pe.SetLifetime(0.35F, 1.05F);
        pe.SetStartEndSize(0.11F, 0.02F);
        pe.SetStartEndColor(
                Vector4{0.35F, 0.95F, 1.0F, 1.0F}, Vector4{0.85F, 0.25F, 1.0F, 0.0F});
        pe.SetGravity({0.0F, 0.18F, 0.0F});
        pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
        pe.SetSpreadAngleRadians(2.05F);
        pe.SetSpeedRange(1.8F, 5.2F);
        break;
    }
    pe.SetMaxParticles(pe.GetMaxParticles());
}

}  // namespace Detail

/**
 * Hit-tests only the right-hand strip (not the full viewport) so the scene stays interactive on the left.
 * One child is arranged to fill that strip (typically a shaded Panel).
 */
class ParticleEffectsRightDockRoot final : public Spark::Gui::Widget {
public:
    void Arrange(const Spark::Gui::Rect& r) override {
        constexpr float kMargin = 14.0F;
        constexpr float kMinW = 500.0F;
        constexpr float kMaxW = 640.0F;
        const float wMax = std::max(kMinW, r.width - 2.0F * kMargin);
        const float w = std::clamp(r.width * 0.44F, kMinW, std::min(kMaxW, wMax));
        const float h = std::max(160.0F, r.height - 2.0F * kMargin);
        const float x = r.x + std::max(0.0F, r.width - w - kMargin);
        const float y = r.y + kMargin;
        const Spark::Gui::Rect panel{x, y, w, h};
        bounds = panel;
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i]) {
                children[i]->Arrange(panel);
            }
        }
    }
};

/**
 * Child 0: lower body (e.g. StackPanel). Children 1–3: title label, "Emitter" label, emitter picker — arranged in a
 * fixed-height band at the top. The band is painted and hit-tested after the body so the picker is not occluded by
 * sliders that sit lower in layout order.
 */
class ParticleGuiEmitterOverlayLayout final : public Spark::Gui::Widget {
public:
    ParticleGuiEmitterOverlayLayout() { SetHitTest(false); }

    void Arrange(const Spark::Gui::Rect& r) override {
        bounds = r;
        if (children.GetSize() < 4U) {
            return;
        }
        Spark::Gui::Widget* const lower = children[0U].Get();
        Spark::Gui::Widget* const titleW = children[1U].Get();
        Spark::Gui::Widget* const pickW = children[2U].Get();
        Spark::Gui::Widget* const ddW = children[3U].Get();
        if (lower == nullptr || titleW == nullptr || pickW == nullptr || ddW == nullptr) {
            return;
        }
        constexpr float kGap = 9.0F;
        constexpr float kTitleH = 30.0F;
        constexpr float kPickH = 21.0F;
        /** Vertical stack of emitter preset buttons (replaces Dropdown until list rendering is fixed). */
        constexpr float kEmitterPickH = 4.0F * 44.0F + 3.0F * 6.0F;
        constexpr float kUpper = kTitleH + kGap + kPickH + kGap + kEmitterPickH;
        float y = r.y;
        titleW->Arrange({r.x, y, r.width, kTitleH});
        y += kTitleH + kGap;
        pickW->Arrange({r.x, y, r.width, kPickH});
        y += kPickH + kGap;
        ddW->Arrange({r.x, y, r.width, kEmitterPickH});
        y = r.y + kUpper + kGap;
        lower->Arrange({r.x, y, r.width, std::max(0.0F, r.height - (y - r.y))});
    }

    void Paint(Spark::Gui::GuiPaintContext& ctx) const override {
        if (!visible) {
            return;
        }
        if (children.GetSize() >= 4U && children[0U]) {
            children[0U]->Paint(ctx);
        }
        if (children.GetSize() >= 4U && children[1U]) {
            children[1U]->Paint(ctx);
        }
        if (children.GetSize() >= 4U && children[2U]) {
            children[2U]->Paint(ctx);
        }
        if (children.GetSize() >= 4U && children[3U]) {
            children[3U]->Paint(ctx);
        }
    }

    Spark::Gui::Widget* FindDeepestHover(float x, float y) override {
        if (!visible || !enabled) {
            return nullptr;
        }
        for (std::size_t idx = children.GetSize(); idx > 0U; --idx) {
            Spark::Gui::Widget* const c = children[idx - 1U].Get();
            if (c != nullptr) {
                if (Spark::Gui::Widget* const h = c->FindDeepestHover(x, y)) {
                    return h;
                }
            }
        }
        return nullptr;
    }
};

class ParticleDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void ClearGuiWidgetRefs() noexcept;


    [[nodiscard]] Spark::ParticleEmitterComponent* SelectedEmitter() noexcept;


    void SyncGuiFromSelectedEmitter();


    void BuildGuiPanel(Spark::GuiCanvasComponent& canvas);


    Spark::Array<Spark::GameObject*> roots{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::GameObject* groundObject = nullptr;
    Spark::GameObject* cubeObject = nullptr;
    Spark::GameObject* effectObjects[Detail::kParticleDemoEffectCount]{};
    Spark::ParticleEmitterComponent* effectEmitters[Detail::kParticleDemoEffectCount]{};
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::GameObject* guiObject = nullptr;

    int guiSelectedEffect = 0;
    Spark::Gui::Button* guiEffectButtons[Detail::kParticleDemoEffectCount]{};
    Spark::Gui::Slider* guiEmission = nullptr;
    Spark::Gui::Slider* guiLifeMin = nullptr;
    Spark::Gui::Slider* guiLifeMax = nullptr;
    Spark::Gui::Slider* guiSizeStart = nullptr;
    Spark::Gui::Slider* guiSizeEnd = nullptr;
    Spark::Gui::Slider* guiSpread = nullptr;
    Spark::Gui::Slider* guiSpeedMin = nullptr;
    Spark::Gui::Slider* guiSpeedMax = nullptr;
    Spark::Gui::Slider* guiGravY = nullptr;
    Spark::Gui::Switch* guiEnabledSwitch = nullptr;

    float fpsSmoothed = 0.0F;

};

}  // namespace Spark
