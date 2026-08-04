#pragma once

#include "spark/ui/factory/IUiControlsFactory.hpp"
#include "spark/ui/runtime/UiFrameContext.hpp"

namespace Spark::Ui {

/**
 * Facade for one frame of UI construction: active factory + frame parameters.
 * Phase 1+ will expose tree builders; Phase 0 holds factory access only.
 */
struct UiContext {
    IUiControlsFactory* factory = nullptr;
    UiFrameContext frame{};
};

}  // namespace Spark::Ui
