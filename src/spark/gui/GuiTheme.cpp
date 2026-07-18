#include "spark/gui/GuiTheme.hpp"

namespace Spark::Gui {

GuiTheme GuiTheme::ClassicMint() noexcept {
    GuiTheme t{};
    t.controlIdleTop = {0.84F, 0.93F, 0.86F};
    t.controlIdleBottom = {0.70F, 0.84F, 0.72F};
    t.controlHotTop = {0.76F, 0.90F, 0.88F};
    t.controlHotBottom = {0.58F, 0.78F, 0.74F};
    t.controlActiveTop = {0.40F, 0.62F, 0.50F};
    t.controlActiveBottom = {0.30F, 0.50F, 0.40F};
    t.controlAccentTop = {0.34F, 0.72F, 0.58F};
    t.controlAccentBottom = {0.24F, 0.58F, 0.46F};
    t.controlFillAlpha = 0.93F;
    t.controlStrokeAlpha = 0.66F;
    t.controlCornerRadius = 8.0F;
    t.textBoxCornerRadius = 8.0F;

    t.borderRgb = {0.36F, 0.54F, 0.40F};
    t.shadowRgb = {0.06F, 0.14F, 0.10F};

    t.labelPrimary = {0.08F, 0.20F, 0.14F};
    t.labelMuted = {0.32F, 0.46F, 0.36F};
    t.labelOnAccent = {0.96F, 0.99F, 0.96F};

    t.scrollViewportTop = {0.88F, 0.94F, 0.90F};
    t.scrollViewportBottom = {0.72F, 0.86F, 0.76F};
    t.scrollViewportAlpha = 0.96F;

    t.insetTrackRgb = {0.12F, 0.20F, 0.15F};
    t.thumbGradientTop = {0.66F, 0.84F, 0.70F};
    t.thumbGradientBottom = {0.50F, 0.70F, 0.56F};

    t.sliderTrackRgb = {0.12F, 0.20F, 0.15F};
    t.sliderThumbTop = {0.74F, 0.90F, 0.78F};
    t.sliderThumbBottom = {0.54F, 0.74F, 0.60F};

    t.progressTrackRgb = {0.10F, 0.16F, 0.12F};
    t.progressFillTop = {0.30F, 0.74F, 0.56F};
    t.progressFillBottom = {0.18F, 0.58F, 0.44F};

    t.switchTrackOffTop = {0.58F, 0.64F, 0.60F};
    t.switchTrackOffBottom = {0.44F, 0.52F, 0.48F};
    t.switchTrackOnTop = {0.32F, 0.74F, 0.58F};
    t.switchTrackOnBottom = {0.20F, 0.58F, 0.44F};
    t.switchKnobTop = {0.94F, 0.98F, 0.92F};
    t.switchKnobBottom = {0.76F, 0.88F, 0.78F};

    t.checkFrameTop = {0.76F, 0.88F, 0.74F};
    t.checkFrameBottom = {0.62F, 0.78F, 0.62F};
    t.checkFillTop = {0.32F, 0.74F, 0.54F};
    t.checkFillBottom = {0.20F, 0.58F, 0.42F};
    t.checkInnerStrokeRgb = {0.16F, 0.40F, 0.28F};

    t.textBoxFillTop = {0.90F, 0.96F, 0.88F};
    t.textBoxFillBottom = {0.74F, 0.88F, 0.76F};
    t.textBoxFillAlpha = 0.90F;
    t.textBoxBorderFocus = {0.26F, 0.62F, 0.46F};
    t.textBoxBorderIdle = {0.40F, 0.56F, 0.42F};

    t.numericFillTop = {0.84F, 0.94F, 0.82F};
    t.numericFillBottom = {0.68F, 0.84F, 0.70F};
    t.numericBorderDragging = {0.30F, 0.68F, 0.48F};
    t.numericBorderIdle = {0.38F, 0.54F, 0.40F};

    t.dialogDimmerTop = {0.03F, 0.10F, 0.06F};
    t.dialogDimmerBottom = {0.06F, 0.14F, 0.08F};
    t.dialogDimmerAlpha = 0.58F;
    t.dialogTitleText = {0.92F, 0.98F, 0.94F};
    t.panelElevatedTop = {0.86F, 0.94F, 0.84F};
    t.panelElevatedBottom = {0.74F, 0.88F, 0.76F};
    t.panelElevatedAlpha = 0.98F;

    t.tabHeaderTop = {0.22F, 0.34F, 0.26F};
    t.tabHeaderBottom = {0.16F, 0.26F, 0.20F};
    t.tabHeaderAlpha = 0.94F;
    t.tabBodyTop = {0.78F, 0.90F, 0.80F};
    t.tabBodyBottom = {0.64F, 0.80F, 0.66F};
    t.tabBodyAlpha = 0.90F;

    t.dropdownPanelTop = {0.82F, 0.93F, 0.78F};
    t.dropdownPanelBottom = {0.66F, 0.84F, 0.66F};
    t.dropdownPanelAlpha = 0.98F;

    t.shellBackdropTop = {0.88F, 0.94F, 0.90F};
    t.shellBackdropBottom = {0.72F, 0.86F, 0.76F};
    t.shellBackdropAlpha = 0.92F;

    t.albumCardTop = {0.82F, 0.92F, 0.80F};
    t.albumCardBottom = {0.66F, 0.84F, 0.68F};
    t.albumCardAlpha = 0.98F;
    t.albumArtPlaceholderTop = {0.56F, 0.70F, 0.58F};
    t.albumArtPlaceholderBottom = {0.34F, 0.50F, 0.40F};
    t.albumAccentBarRgb = {0.24F, 0.62F, 0.48F};
    t.albumAccentBarAlpha = 1.0F;
    return t;
}

GuiTheme GuiTheme::TwilightSlate() noexcept {
    GuiTheme t{};
    t.controlIdleTop = {0.20F, 0.22F, 0.28F};
    t.controlIdleBottom = {0.14F, 0.16F, 0.21F};
    t.controlHotTop = {0.26F, 0.30F, 0.38F};
    t.controlHotBottom = {0.18F, 0.22F, 0.30F};
    t.controlActiveTop = {0.30F, 0.56F, 0.52F};
    t.controlActiveBottom = {0.22F, 0.44F, 0.42F};
    t.controlAccentTop = {0.28F, 0.72F, 0.66F};
    t.controlAccentBottom = {0.18F, 0.56F, 0.52F};
    t.controlFillAlpha = 0.98F;
    t.controlStrokeAlpha = 0.70F;
    t.controlCornerRadius = 8.0F;
    t.textBoxCornerRadius = 8.0F;

    t.borderRgb = {0.38F, 0.44F, 0.52F};
    t.shadowRgb = {0.03F, 0.04F, 0.06F};

    t.labelPrimary = {0.92F, 0.94F, 0.96F};
    t.labelMuted = {0.62F, 0.68F, 0.74F};
    t.labelOnAccent = {0.06F, 0.10F, 0.12F};

    t.scrollViewportTop = {0.18F, 0.20F, 0.26F};
    t.scrollViewportBottom = {0.12F, 0.14F, 0.19F};
    t.scrollViewportAlpha = 0.98F;

    t.insetTrackRgb = {0.08F, 0.10F, 0.13F};
    t.thumbGradientTop = {0.32F, 0.38F, 0.46F};
    t.thumbGradientBottom = {0.22F, 0.28F, 0.36F};

    t.sliderTrackRgb = {0.08F, 0.10F, 0.13F};
    t.sliderThumbTop = {0.34F, 0.42F, 0.50F};
    t.sliderThumbBottom = {0.24F, 0.32F, 0.40F};

    t.progressTrackRgb = {0.08F, 0.10F, 0.13F};
    t.progressFillTop = {0.30F, 0.74F, 0.68F};
    t.progressFillBottom = {0.18F, 0.58F, 0.54F};

    t.switchTrackOffTop = {0.22F, 0.24F, 0.30F};
    t.switchTrackOffBottom = {0.16F, 0.18F, 0.24F};
    t.switchTrackOnTop = {0.28F, 0.72F, 0.66F};
    t.switchTrackOnBottom = {0.18F, 0.56F, 0.52F};
    t.switchKnobTop = {0.90F, 0.92F, 0.94F};
    t.switchKnobBottom = {0.72F, 0.76F, 0.82F};

    t.checkFrameTop = {0.20F, 0.22F, 0.28F};
    t.checkFrameBottom = {0.14F, 0.16F, 0.21F};
    t.checkFillTop = {0.28F, 0.72F, 0.66F};
    t.checkFillBottom = {0.18F, 0.56F, 0.52F};
    t.checkInnerStrokeRgb = {0.06F, 0.10F, 0.12F};

    t.textBoxFillTop = {0.18F, 0.20F, 0.26F};
    t.textBoxFillBottom = {0.12F, 0.14F, 0.19F};
    t.textBoxFillAlpha = 0.98F;
    t.textBoxBorderFocus = {0.34F, 0.74F, 0.68F};
    t.textBoxBorderIdle = {0.32F, 0.38F, 0.46F};

    t.numericFillTop = {0.18F, 0.20F, 0.26F};
    t.numericFillBottom = {0.12F, 0.14F, 0.19F};
    t.numericBorderDragging = {0.34F, 0.74F, 0.68F};
    t.numericBorderIdle = {0.32F, 0.38F, 0.46F};

    t.dialogDimmerTop = {0.02F, 0.03F, 0.05F};
    t.dialogDimmerBottom = {0.04F, 0.06F, 0.09F};
    t.dialogDimmerAlpha = 0.64F;
    t.dialogTitleText = {0.92F, 0.94F, 0.96F};
    t.panelElevatedTop = {0.22F, 0.24F, 0.30F};
    t.panelElevatedBottom = {0.14F, 0.16F, 0.21F};
    t.panelElevatedAlpha = 0.98F;

    t.tabHeaderTop = {0.10F, 0.12F, 0.16F};
    t.tabHeaderBottom = {0.06F, 0.08F, 0.11F};
    t.tabHeaderAlpha = 0.96F;
    t.tabBodyTop = {0.18F, 0.20F, 0.26F};
    t.tabBodyBottom = {0.12F, 0.14F, 0.19F};
    t.tabBodyAlpha = 0.96F;

    t.dropdownPanelTop = {0.20F, 0.22F, 0.28F};
    t.dropdownPanelBottom = {0.14F, 0.16F, 0.21F};
    t.dropdownPanelAlpha = 0.98F;

    t.shellBackdropTop = {0.14F, 0.16F, 0.22F};
    t.shellBackdropBottom = {0.08F, 0.10F, 0.14F};
    t.shellBackdropAlpha = 0.94F;

    t.albumCardTop = {0.20F, 0.22F, 0.28F};
    t.albumCardBottom = {0.14F, 0.16F, 0.21F};
    t.albumCardAlpha = 0.98F;
    t.albumArtPlaceholderTop = {0.28F, 0.32F, 0.40F};
    t.albumArtPlaceholderBottom = {0.18F, 0.22F, 0.30F};
    t.albumAccentBarRgb = {0.92F, 0.52F, 0.44F};
    t.albumAccentBarAlpha = 1.0F;
    return t;
}

GuiTheme GuiTheme::SceneEditorDark() noexcept {
    GuiTheme t{};
    t.controlIdleTop = {0.22F, 0.24F, 0.30F};
    t.controlIdleBottom = {0.17F, 0.19F, 0.25F};
    t.controlHotTop = {0.28F, 0.32F, 0.40F};
    t.controlHotBottom = {0.22F, 0.26F, 0.34F};
    t.controlActiveTop = {0.34F, 0.44F, 0.58F};
    t.controlActiveBottom = {0.26F, 0.34F, 0.48F};
    t.controlAccentTop = {0.36F, 0.48F, 0.66F};
    t.controlAccentBottom = {0.26F, 0.36F, 0.52F};
    t.controlFillAlpha = 1.0F;
    t.controlStrokeAlpha = 0.72F;
    t.controlCornerRadius = 6.0F;
    t.textBoxCornerRadius = 6.0F;

    t.borderRgb = {0.40F, 0.44F, 0.52F};
    t.shadowRgb = {0.04F, 0.05F, 0.08F};

    t.labelPrimary = {0.92F, 0.94F, 0.98F};
    t.labelMuted = {0.62F, 0.68F, 0.78F};
    t.labelOnAccent = {0.96F, 0.98F, 1.0F};

    t.scrollViewportTop = {0.16F, 0.18F, 0.25F};
    t.scrollViewportBottom = {0.10F, 0.11F, 0.16F};
    t.scrollViewportAlpha = 1.0F;

    t.insetTrackRgb = {0.10F, 0.11F, 0.14F};
    t.thumbGradientTop = {0.30F, 0.34F, 0.42F};
    t.thumbGradientBottom = {0.22F, 0.26F, 0.34F};

    t.sliderTrackRgb = {0.10F, 0.11F, 0.14F};
    t.sliderThumbTop = {0.32F, 0.38F, 0.48F};
    t.sliderThumbBottom = {0.24F, 0.30F, 0.40F};

    t.progressTrackRgb = {0.10F, 0.11F, 0.14F};
    t.progressFillTop = {0.36F, 0.48F, 0.66F};
    t.progressFillBottom = {0.26F, 0.36F, 0.52F};

    t.switchTrackOffTop = {0.20F, 0.22F, 0.28F};
    t.switchTrackOffBottom = {0.16F, 0.18F, 0.24F};
    t.switchTrackOnTop = {0.32F, 0.44F, 0.58F};
    t.switchTrackOnBottom = {0.24F, 0.34F, 0.48F};
    t.switchKnobTop = {0.88F, 0.90F, 0.94F};
    t.switchKnobBottom = {0.72F, 0.76F, 0.84F};

    t.checkFrameTop = {0.22F, 0.24F, 0.30F};
    t.checkFrameBottom = {0.17F, 0.19F, 0.25F};
    t.checkFillTop = {0.36F, 0.48F, 0.66F};
    t.checkFillBottom = {0.26F, 0.36F, 0.52F};
    t.checkInnerStrokeRgb = {0.48F, 0.58F, 0.72F};

    t.textBoxFillTop = {0.20F, 0.22F, 0.28F};
    t.textBoxFillBottom = {0.16F, 0.18F, 0.24F};
    t.textBoxFillAlpha = 1.0F;
    t.textBoxBorderFocus = {0.42F, 0.56F, 0.74F};
    t.textBoxBorderIdle = {0.34F, 0.38F, 0.46F};

    t.numericFillTop = {0.20F, 0.22F, 0.28F};
    t.numericFillBottom = {0.16F, 0.18F, 0.24F};
    t.numericBorderDragging = {0.42F, 0.56F, 0.74F};
    t.numericBorderIdle = {0.34F, 0.38F, 0.46F};

    t.dialogDimmerTop = {0.02F, 0.03F, 0.05F};
    t.dialogDimmerBottom = {0.04F, 0.06F, 0.09F};
    t.dialogDimmerAlpha = 0.62F;
    t.dialogTitleText = {0.92F, 0.94F, 0.98F};
    t.panelElevatedTop = {0.24F, 0.26F, 0.32F};
    t.panelElevatedBottom = {0.16F, 0.18F, 0.24F};
    t.panelElevatedAlpha = 0.98F;

    t.tabHeaderTop = {0.12F, 0.13F, 0.17F};
    t.tabHeaderBottom = {0.08F, 0.09F, 0.12F};
    t.tabHeaderAlpha = 0.96F;
    t.tabBodyTop = {0.20F, 0.22F, 0.28F};
    t.tabBodyBottom = {0.16F, 0.18F, 0.24F};
    t.tabBodyAlpha = 0.96F;

    t.dropdownPanelTop = {0.22F, 0.24F, 0.30F};
    t.dropdownPanelBottom = {0.16F, 0.18F, 0.24F};
    t.dropdownPanelAlpha = 0.98F;

    t.shellBackdropTop = {0.10F, 0.11F, 0.15F};
    t.shellBackdropBottom = {0.06F, 0.07F, 0.10F};
    t.shellBackdropAlpha = 0.92F;

    t.albumCardTop = {0.20F, 0.22F, 0.28F};
    t.albumCardBottom = {0.16F, 0.18F, 0.24F};
    t.albumCardAlpha = 0.98F;
    t.albumArtPlaceholderTop = {0.26F, 0.30F, 0.38F};
    t.albumArtPlaceholderBottom = {0.18F, 0.22F, 0.30F};
    t.albumAccentBarRgb = {0.36F, 0.48F, 0.66F};
    t.albumAccentBarAlpha = 1.0F;
    return t;
}

namespace {

GuiTheme MakeHighContrastBase() noexcept {
    GuiTheme t{};
    t.controlFillAlpha = 1.0F;
    t.controlStrokeAlpha = 1.0F;
    t.controlCornerRadius = 2.0F;
    t.textBoxCornerRadius = 2.0F;
    t.scrollViewportAlpha = 1.0F;
    t.textBoxFillAlpha = 1.0F;
    t.dialogDimmerAlpha = 0.85F;
    t.panelElevatedAlpha = 1.0F;
    t.tabHeaderAlpha = 1.0F;
    t.tabBodyAlpha = 1.0F;
    t.dropdownPanelAlpha = 1.0F;
    t.shellBackdropAlpha = 1.0F;
    t.albumCardAlpha = 1.0F;
    t.albumAccentBarAlpha = 1.0F;
    return t;
}

}  // namespace

GuiTheme GuiTheme::HighContrastLight() noexcept {
    GuiTheme t = MakeHighContrastBase();
    t.controlIdleTop = {1.0F, 1.0F, 1.0F};
    t.controlIdleBottom = {0.92F, 0.92F, 0.92F};
    t.controlHotTop = {0.86F, 0.92F, 1.0F};
    t.controlHotBottom = {0.72F, 0.82F, 1.0F};
    t.controlActiveTop = {0.60F, 0.78F, 1.0F};
    t.controlActiveBottom = {0.42F, 0.62F, 0.98F};
    t.controlAccentTop = {0.0F, 0.0F, 0.78F};
    t.controlAccentBottom = {0.0F, 0.0F, 0.62F};

    t.borderRgb = {0.0F, 0.0F, 0.0F};
    t.shadowRgb = {0.0F, 0.0F, 0.0F};

    t.labelPrimary = {0.0F, 0.0F, 0.0F};
    t.labelMuted = {0.22F, 0.22F, 0.22F};
    t.labelOnAccent = {1.0F, 1.0F, 1.0F};

    t.scrollViewportTop = {1.0F, 1.0F, 1.0F};
    t.scrollViewportBottom = {0.94F, 0.94F, 0.94F};
    t.insetTrackRgb = {0.82F, 0.82F, 0.82F};
    t.thumbGradientTop = {0.0F, 0.0F, 0.78F};
    t.thumbGradientBottom = {0.0F, 0.0F, 0.62F};

    t.sliderTrackRgb = {0.82F, 0.82F, 0.82F};
    t.sliderThumbTop = {0.0F, 0.0F, 0.78F};
    t.sliderThumbBottom = {0.0F, 0.0F, 0.62F};

    t.progressTrackRgb = {0.82F, 0.82F, 0.82F};
    t.progressFillTop = {0.0F, 0.0F, 0.78F};
    t.progressFillBottom = {0.0F, 0.0F, 0.62F};

    t.switchTrackOffTop = {0.88F, 0.88F, 0.88F};
    t.switchTrackOffBottom = {0.72F, 0.72F, 0.72F};
    t.switchTrackOnTop = {0.0F, 0.0F, 0.78F};
    t.switchTrackOnBottom = {0.0F, 0.0F, 0.62F};
    t.switchKnobTop = {1.0F, 1.0F, 1.0F};
    t.switchKnobBottom = {0.92F, 0.92F, 0.92F};

    t.checkFrameTop = {1.0F, 1.0F, 1.0F};
    t.checkFrameBottom = {0.92F, 0.92F, 0.92F};
    t.checkFillTop = {0.0F, 0.0F, 0.78F};
    t.checkFillBottom = {0.0F, 0.0F, 0.62F};
    t.checkInnerStrokeRgb = {1.0F, 1.0F, 1.0F};

    t.textBoxFillTop = {1.0F, 1.0F, 1.0F};
    t.textBoxFillBottom = {0.96F, 0.96F, 0.96F};
    t.textBoxBorderFocus = {0.0F, 0.0F, 0.78F};
    t.textBoxBorderIdle = {0.0F, 0.0F, 0.0F};

    t.numericFillTop = {1.0F, 1.0F, 1.0F};
    t.numericFillBottom = {0.96F, 0.96F, 0.96F};
    t.numericBorderDragging = {0.0F, 0.0F, 0.78F};
    t.numericBorderIdle = {0.0F, 0.0F, 0.0F};

    t.dialogDimmerTop = {0.0F, 0.0F, 0.0F};
    t.dialogDimmerBottom = {0.0F, 0.0F, 0.0F};
    t.dialogTitleText = {1.0F, 1.0F, 1.0F};
    t.panelElevatedTop = {1.0F, 1.0F, 1.0F};
    t.panelElevatedBottom = {0.94F, 0.94F, 0.94F};

    t.tabHeaderTop = {0.92F, 0.92F, 0.92F};
    t.tabHeaderBottom = {0.86F, 0.86F, 0.86F};
    t.tabBodyTop = {1.0F, 1.0F, 1.0F};
    t.tabBodyBottom = {0.98F, 0.98F, 0.98F};

    t.dropdownPanelTop = {1.0F, 1.0F, 1.0F};
    t.dropdownPanelBottom = {0.94F, 0.94F, 0.94F};

    t.shellBackdropTop = {1.0F, 1.0F, 1.0F};
    t.shellBackdropBottom = {0.94F, 0.94F, 0.94F};

    t.albumCardTop = {1.0F, 1.0F, 1.0F};
    t.albumCardBottom = {0.94F, 0.94F, 0.94F};
    t.albumArtPlaceholderTop = {0.86F, 0.86F, 0.86F};
    t.albumArtPlaceholderBottom = {0.72F, 0.72F, 0.72F};
    t.albumAccentBarRgb = {0.0F, 0.0F, 0.78F};
    return t;
}

GuiTheme GuiTheme::HighContrastDark() noexcept {
    GuiTheme t = MakeHighContrastBase();
    t.controlIdleTop = {0.12F, 0.12F, 0.12F};
    t.controlIdleBottom = {0.0F, 0.0F, 0.0F};
    t.controlHotTop = {0.24F, 0.24F, 0.24F};
    t.controlHotBottom = {0.12F, 0.12F, 0.12F};
    t.controlActiveTop = {0.42F, 0.42F, 0.42F};
    t.controlActiveBottom = {0.28F, 0.28F, 0.28F};
    t.controlAccentTop = {1.0F, 1.0F, 1.0F};
    t.controlAccentBottom = {0.82F, 0.82F, 0.82F};

    t.borderRgb = {1.0F, 1.0F, 1.0F};
    t.shadowRgb = {0.0F, 0.0F, 0.0F};

    t.labelPrimary = {1.0F, 1.0F, 1.0F};
    t.labelMuted = {0.78F, 0.78F, 0.78F};
    t.labelOnAccent = {0.0F, 0.0F, 0.0F};

    t.scrollViewportTop = {0.08F, 0.08F, 0.08F};
    t.scrollViewportBottom = {0.0F, 0.0F, 0.0F};
    t.insetTrackRgb = {0.22F, 0.22F, 0.22F};
    t.thumbGradientTop = {1.0F, 1.0F, 1.0F};
    t.thumbGradientBottom = {0.82F, 0.82F, 0.82F};

    t.sliderTrackRgb = {0.22F, 0.22F, 0.22F};
    t.sliderThumbTop = {1.0F, 1.0F, 1.0F};
    t.sliderThumbBottom = {0.82F, 0.82F, 0.82F};

    t.progressTrackRgb = {0.22F, 0.22F, 0.22F};
    t.progressFillTop = {1.0F, 1.0F, 1.0F};
    t.progressFillBottom = {0.82F, 0.82F, 0.82F};

    t.switchTrackOffTop = {0.22F, 0.22F, 0.22F};
    t.switchTrackOffBottom = {0.12F, 0.12F, 0.12F};
    t.switchTrackOnTop = {1.0F, 1.0F, 1.0F};
    t.switchTrackOnBottom = {0.82F, 0.82F, 0.82F};
    t.switchKnobTop = {0.0F, 0.0F, 0.0F};
    t.switchKnobBottom = {0.12F, 0.12F, 0.12F};

    t.checkFrameTop = {0.12F, 0.12F, 0.12F};
    t.checkFrameBottom = {0.0F, 0.0F, 0.0F};
    t.checkFillTop = {1.0F, 1.0F, 1.0F};
    t.checkFillBottom = {0.82F, 0.82F, 0.82F};
    t.checkInnerStrokeRgb = {0.0F, 0.0F, 0.0F};

    t.textBoxFillTop = {0.0F, 0.0F, 0.0F};
    t.textBoxFillBottom = {0.08F, 0.08F, 0.08F};
    t.textBoxBorderFocus = {1.0F, 1.0F, 0.0F};
    t.textBoxBorderIdle = {1.0F, 1.0F, 1.0F};

    t.numericFillTop = {0.0F, 0.0F, 0.0F};
    t.numericFillBottom = {0.08F, 0.08F, 0.08F};
    t.numericBorderDragging = {1.0F, 1.0F, 0.0F};
    t.numericBorderIdle = {1.0F, 1.0F, 1.0F};

    t.dialogDimmerTop = {0.0F, 0.0F, 0.0F};
    t.dialogDimmerBottom = {0.0F, 0.0F, 0.0F};
    t.dialogTitleText = {1.0F, 1.0F, 1.0F};
    t.panelElevatedTop = {0.12F, 0.12F, 0.12F};
    t.panelElevatedBottom = {0.0F, 0.0F, 0.0F};

    t.tabHeaderTop = {0.0F, 0.0F, 0.0F};
    t.tabHeaderBottom = {0.0F, 0.0F, 0.0F};
    t.tabBodyTop = {0.08F, 0.08F, 0.08F};
    t.tabBodyBottom = {0.0F, 0.0F, 0.0F};

    t.dropdownPanelTop = {0.0F, 0.0F, 0.0F};
    t.dropdownPanelBottom = {0.08F, 0.08F, 0.08F};

    t.shellBackdropTop = {0.0F, 0.0F, 0.0F};
    t.shellBackdropBottom = {0.0F, 0.0F, 0.0F};

    t.albumCardTop = {0.08F, 0.08F, 0.08F};
    t.albumCardBottom = {0.0F, 0.0F, 0.0F};
    t.albumArtPlaceholderTop = {0.22F, 0.22F, 0.22F};
    t.albumArtPlaceholderBottom = {0.12F, 0.12F, 0.12F};
    t.albumAccentBarRgb = {1.0F, 1.0F, 1.0F};
    return t;
}

GuiTheme GuiTheme::HighContrastYellowOnBlack() noexcept {
    GuiTheme t = MakeHighContrastBase();
    const Vector3 yellow{1.0F, 1.0F, 0.0F};
    const Vector3 black{0.0F, 0.0F, 0.0F};
    const Vector3 dimYellow{0.72F, 0.72F, 0.0F};

    t.controlIdleTop = black;
    t.controlIdleBottom = black;
    t.controlHotTop = {0.18F, 0.18F, 0.0F};
    t.controlHotBottom = black;
    t.controlActiveTop = {0.32F, 0.32F, 0.0F};
    t.controlActiveBottom = {0.18F, 0.18F, 0.0F};
    t.controlAccentTop = yellow;
    t.controlAccentBottom = dimYellow;

    t.borderRgb = yellow;
    t.shadowRgb = black;

    t.labelPrimary = yellow;
    t.labelMuted = dimYellow;
    t.labelOnAccent = black;

    t.scrollViewportTop = black;
    t.scrollViewportBottom = black;
    t.insetTrackRgb = {0.22F, 0.22F, 0.0F};
    t.thumbGradientTop = yellow;
    t.thumbGradientBottom = dimYellow;

    t.sliderTrackRgb = {0.22F, 0.22F, 0.0F};
    t.sliderThumbTop = yellow;
    t.sliderThumbBottom = dimYellow;

    t.progressTrackRgb = {0.22F, 0.22F, 0.0F};
    t.progressFillTop = yellow;
    t.progressFillBottom = dimYellow;

    t.switchTrackOffTop = {0.22F, 0.22F, 0.0F};
    t.switchTrackOffBottom = black;
    t.switchTrackOnTop = yellow;
    t.switchTrackOnBottom = dimYellow;
    t.switchKnobTop = black;
    t.switchKnobBottom = {0.12F, 0.12F, 0.0F};

    t.checkFrameTop = black;
    t.checkFrameBottom = black;
    t.checkFillTop = yellow;
    t.checkFillBottom = dimYellow;
    t.checkInnerStrokeRgb = black;

    t.textBoxFillTop = black;
    t.textBoxFillBottom = black;
    t.textBoxBorderFocus = yellow;
    t.textBoxBorderIdle = yellow;

    t.numericFillTop = black;
    t.numericFillBottom = black;
    t.numericBorderDragging = yellow;
    t.numericBorderIdle = yellow;

    t.dialogDimmerTop = black;
    t.dialogDimmerBottom = black;
    t.dialogTitleText = yellow;
    t.panelElevatedTop = black;
    t.panelElevatedBottom = black;

    t.tabHeaderTop = black;
    t.tabHeaderBottom = black;
    t.tabBodyTop = black;
    t.tabBodyBottom = black;

    t.dropdownPanelTop = black;
    t.dropdownPanelBottom = black;

    t.shellBackdropTop = black;
    t.shellBackdropBottom = black;

    t.albumCardTop = black;
    t.albumCardBottom = black;
    t.albumArtPlaceholderTop = {0.22F, 0.22F, 0.0F};
    t.albumArtPlaceholderBottom = black;
    t.albumAccentBarRgb = yellow;
    return t;
}

}  // namespace Spark::Gui
