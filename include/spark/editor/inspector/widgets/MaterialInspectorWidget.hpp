#pragma once

#include "spark/core/Array.hpp"
#include "spark/editor/commands/SetMaterialInspectorCommand.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

enum class MaterialTextureSlot : int {
    BaseColor = 0,
    Normal = 1,
    MetallicRoughness = 2,
    Emissive = 3,
};

class MaterialInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::Material; }
    [[nodiscard]] bool IsRelevantFor(const GameObject* target) const noexcept override;

    void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) override;
    void SetSectionVisible(bool visible) override;
    void SyncFromTarget(const InspectorWidgetContext& ctx) override;
    void CommitPendingEdits(const InspectorWidgetContext& ctx) override;
    void CancelPendingEdits() override;
    void PrepareFrameContext(const InspectorWidgetContext& ctx) override;
    [[nodiscard]] bool HasPendingEdits() const noexcept override { return editTracker.HasPendingEdit(); }

private:
    void OnTintChanged(int axis, float value);
    void OnMetallicChanged(float value);
    void OnRoughnessChanged(float value);
    void OnEmissiveIntensityChanged(float value);
    void OnTextureSlotChanged(MaterialTextureSlot slot, int listIndex);
    void ApplyLiveEdit();
    void RefreshTextureLists();
    void SyncTextureListSelections();
    [[nodiscard]] Utf8String* TexturePathForSlot(MaterialTextureSlot slot) noexcept;
    [[nodiscard]] const Utf8String* TexturePathForSlot(MaterialTextureSlot slot) const noexcept;

    static void OnTintRChanged(void* userData, float value);
    static void OnTintGChanged(void* userData, float value);
    static void OnTintBChanged(void* userData, float value);
    static void OnMetallicSliderChanged(void* userData, float value);
    static void OnRoughnessSliderChanged(void* userData, float value);
    static void OnEmissiveIntensitySliderChanged(void* userData, float value);
    static void OnBaseColorTextureChanged(void* userData, int index);
    static void OnNormalTextureChanged(void* userData, int index);
    static void OnMetallicRoughnessTextureChanged(void* userData, int index);
    static void OnEmissiveTextureChanged(void* userData, int index);

    Ui::IPanel* sectionPanel = nullptr;
    Ui::IList* baseColorTextureList = nullptr;
    Ui::IList* normalTextureList = nullptr;
    Ui::IList* metallicRoughnessTextureList = nullptr;
    Ui::IList* emissiveTextureList = nullptr;
    Array<InspectorSliderBinding> bindings{};
    InspectorEditTracker<MaterialInspectorState> editTracker{};
    InspectorWidgetContext activeCtx{};
    MaterialInspectorState liveState{};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
