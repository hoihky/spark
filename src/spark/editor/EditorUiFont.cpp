#include "spark/editor/EditorUiFont.hpp"

#include "spark/config.hpp"
#include "spark/text/Font.hpp"
#include "spark/scene/GameWorld.hpp"

#include <iostream>
#include <print>

namespace Spark {

void MountEditorUiFonts(GameWorld& world) {
    auto uiFont = MakeShared<Font>();
    constexpr float kUiFontEmPx = 42.0F;
    bool fontOk = uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, kUiFontEmPx);
    if (!fontOk) {
        Utf8String buildTree(SPARK_BUILD_ASSETS_DIR);
        buildTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(buildTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        Utf8String srcTree(SPARK_ASSETS_DIR);
        srcTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(srcTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        fontOk = uiFont->TryLoadTrueTypeFromFile("assets/fonts/Roboto-Regular.ttf", kUiFontEmPx);
    }
    if (!fontOk) {
        std::println(
                std::cerr,
                "Spark Editor: UI font not loaded — place Roboto-Regular.ttf under assets/fonts/ "
                "or re-run CMake (tried SPARK_UI_FONT_PATH, build assets, source assets).");
        return;
    }

    world.SetUiFont(uiFont);

    auto uiBold = MakeShared<Font>();
    bool boldOk = uiBold->TryLoadTrueTypeFromFile(SPARK_UI_BOLD_FONT_PATH, kUiFontEmPx);
    if (!boldOk) {
        Utf8String boldBuild(SPARK_BUILD_ASSETS_DIR);
        boldBuild.AppendUtf8("/fonts/Roboto-Bold.ttf");
        boldOk = uiBold->TryLoadTrueTypeFromFile(boldBuild.CStr(), kUiFontEmPx);
    }
    if (!boldOk) {
        Utf8String boldSrc(SPARK_ASSETS_DIR);
        boldSrc.AppendUtf8("/fonts/Roboto-Bold.ttf");
        boldOk = uiBold->TryLoadTrueTypeFromFile(boldSrc.CStr(), kUiFontEmPx);
    }
    if (!boldOk) {
        boldOk = uiBold->TryLoadTrueTypeFromFile("assets/fonts/Roboto-Bold.ttf", kUiFontEmPx);
    }
    if (boldOk) {
        world.SetUiBoldFont(uiBold);
    }
}

}  // namespace Spark
