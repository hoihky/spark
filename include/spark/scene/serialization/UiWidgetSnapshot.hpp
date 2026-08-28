#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiElement.hpp"

namespace Spark::Ui {

class SparkUiControlsFactory;

/** Serializes common Spark UI widget trees for <c>UiCanvasComponent</c> scene snapshots. */
class UiWidgetSnapshot {
public:
    [[nodiscard]] static bool TryCaptureTree(const IUiElement* root, Utf8String& outPayload);
    [[nodiscard]] static UniquePtr<IUiElement> TryRestoreTree(const char* payload, SparkUiControlsFactory& factory);
};

}  // namespace Spark::Ui
