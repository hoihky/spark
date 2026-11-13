#include "spark/scene/core/GameWorldPresentation.hpp"

#include "spark/scene/core/GameWorld.hpp"
#include "spark/text/Font.hpp"

namespace Spark {

void GameWorldPresentation::Shutdown(GameWorld& world) {
    vfxSubsystem.Shutdown(world);
}

void GameWorldPresentation::SetUiFont(SharedPtr<Font> font) {
    uiFont = MoveTemp(font);
}

void GameWorldPresentation::SetUiBoldFont(SharedPtr<Font> font) {
    uiBoldFont = MoveTemp(font);
}

}  // namespace Spark
