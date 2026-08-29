#pragma once

namespace Spark::Ui {
struct SceneEditorLayoutSettings;
}

namespace Spark::Editor {

/**
 * Built-in editor dock layout: hierarchy + project (left), scene view (center), inspector (right).
 */
struct EditorDefaultLayout final {
    static constexpr float kLeftWidthPx = 280.0F;
    static constexpr float kRightWidthPx = 340.0F;
    /** Hierarchy occupies the top fraction of the left column; project uses the remainder. */
    static constexpr float kLeftStackSplit = 0.55F;
    static constexpr float kToolbarHeightPx = 40.0F;

    static void ApplyTo(Ui::SceneEditorLayoutSettings& settings) noexcept;
};

}  // namespace Spark::Editor
