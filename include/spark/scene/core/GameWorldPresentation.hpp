#pragma once

#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/foliage/WindSubsystem.hpp"
#include "spark/scene/submit/SkinnedAnimationService.hpp"
#include "spark/scene/submit/SkinnedIkService.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark {

class Font;
class GameWorld;

/** VFX, wind, skinned services, and UI fonts for one world. */
class GameWorldPresentation final {
public:
    void Shutdown(GameWorld& world);

    [[nodiscard]] VfxSubsystem& Vfx() noexcept { return vfxSubsystem; }
    [[nodiscard]] const VfxSubsystem& Vfx() const noexcept { return vfxSubsystem; }

    [[nodiscard]] WindSubsystem& Wind() noexcept { return windSubsystem; }
    [[nodiscard]] const WindSubsystem& Wind() const noexcept { return windSubsystem; }

    [[nodiscard]] SkinnedAnimationService& SkinnedAnimation() noexcept { return skinnedAnimationService; }
    [[nodiscard]] const SkinnedAnimationService& SkinnedAnimation() const noexcept { return skinnedAnimationService; }

    [[nodiscard]] SkinnedIkService& SkinnedIk() noexcept { return skinnedIkService; }
    [[nodiscard]] const SkinnedIkService& SkinnedIk() const noexcept { return skinnedIkService; }

    void SetUiFont(SharedPtr<Font> font);
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return uiFont; }
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return uiBoldFont; }

private:
    VfxSubsystem vfxSubsystem{};
    WindSubsystem windSubsystem{};
    SkinnedAnimationService skinnedAnimationService{};
    SkinnedIkService skinnedIkService{};
    SharedPtr<Font> uiFont{};
    SharedPtr<Font> uiBoldFont{};
};

}  // namespace Spark
