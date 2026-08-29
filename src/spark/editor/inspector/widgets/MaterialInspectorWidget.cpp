#include "spark/editor/inspector/widgets/MaterialInspectorWidget.hpp"

#include "spark/editor/EditorTextureCatalog.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Editor {

bool MaterialInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<MaterialComponent>();
}

void MaterialInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    bindings.Reserve(8U);

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_material_section");
    sectionDesc.title = Utf8String("Material");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "Material");
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_tint_r", "Tint R", 1.0F, 0.0F, 1.0F,
            OnTintRChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_tint_g", "Tint G", 1.0F, 0.0F, 1.0F,
            OnTintGChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_tint_b", "Tint B", 1.0F, 0.0F, 1.0F,
            OnTintBChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_metallic", "Metallic", 0.0F, 0.0F, 1.0F,
            OnMetallicSliderChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_roughness", "Roughness", 0.45F, 0.0F, 1.0F,
            OnRoughnessSliderChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_material_emissive", "Emissive", 0.0F, 0.0F, 10.0F,
            OnEmissiveIntensitySliderChanged, this);

    auto makeTextureList = [&](const char* id, Ui::IList*& outList, void (*cb)(void*, int)) {
        InspectorUiBuilder::AddMutedLabel(*sectionPanel, factory, id);
        Ui::ListDesc listDesc{};
        listDesc.id = Utf8String(id);
        listDesc.rowHeight = 22.0F;
        auto listUp = factory.CreateList(listDesc);
        outList = listUp.Get();
        Ui::UiIntCallback listCb{};
        listCb.fn = cb;
        listCb.userData = this;
        outList->SetOnSelectionChanged(listCb);
        AdoptUiChild(*sectionPanel, MoveTemp(listUp));
    };

    makeTextureList("Base Color Texture", baseColorTextureList, OnBaseColorTextureChanged);
    makeTextureList("Normal Texture", normalTextureList, OnNormalTextureChanged);
    makeTextureList("Metallic/Roughness Texture", metallicRoughnessTextureList, OnMetallicRoughnessTextureChanged);
    makeTextureList("Emissive Texture", emissiveTextureList, OnEmissiveTextureChanged);

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void MaterialInspectorWidget::SetSectionVisible(const bool visible) {
    if (sectionPanel != nullptr) {
        sectionPanel->SetVisible(visible);
    }
}

void MaterialInspectorWidget::RefreshTextureLists() {
    Array<Utf8String> items;
    items.PushBack(Utf8String("(None)"));
    if (activeCtx.textureCatalog != nullptr) {
        const Array<EditorTextureEntry>& entries = activeCtx.textureCatalog->GetEntries();
        for (std::size_t i = 0; i < entries.GetSize(); ++i) {
            items.PushBack(entries[i].displayName);
        }
    }
    if (baseColorTextureList != nullptr) {
        baseColorTextureList->SetItems(items);
    }
    if (normalTextureList != nullptr) {
        normalTextureList->SetItems(items);
    }
    if (metallicRoughnessTextureList != nullptr) {
        metallicRoughnessTextureList->SetItems(items);
    }
    if (emissiveTextureList != nullptr) {
        emissiveTextureList->SetItems(items);
    }
}

void MaterialInspectorWidget::SyncTextureListSelections() {
    if (activeCtx.textureCatalog == nullptr) {
        return;
    }
    const EditorTextureCatalog& catalog = *activeCtx.textureCatalog;
    if (baseColorTextureList != nullptr) {
        baseColorTextureList->SetSelectedIndex(catalog.FindIndexByRelativePath(liveState.baseColorTexturePath.CStr()));
    }
    if (normalTextureList != nullptr) {
        normalTextureList->SetSelectedIndex(catalog.FindIndexByRelativePath(liveState.normalTexturePath.CStr()));
    }
    if (metallicRoughnessTextureList != nullptr) {
        metallicRoughnessTextureList->SetSelectedIndex(
                catalog.FindIndexByRelativePath(liveState.metallicRoughnessTexturePath.CStr()));
    }
    if (emissiveTextureList != nullptr) {
        emissiveTextureList->SetSelectedIndex(catalog.FindIndexByRelativePath(liveState.emissiveTexturePath.CStr()));
    }
}

void MaterialInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    if (!built || suppressSync || editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    liveState = SetMaterialInspectorCommand::Capture(*ctx.target);
    RefreshTextureLists();
    InspectorUiBuilder::SetSliderValue(bindings[0].slider, liveState.tint.x);
    InspectorUiBuilder::SetSliderValue(bindings[1].slider, liveState.tint.y);
    InspectorUiBuilder::SetSliderValue(bindings[2].slider, liveState.tint.z);
    InspectorUiBuilder::SetSliderValue(bindings[3].slider, liveState.metallic);
    InspectorUiBuilder::SetSliderValue(bindings[4].slider, liveState.roughness);
    InspectorUiBuilder::SetSliderValue(bindings[5].slider, liveState.emissiveIntensity);
    SyncTextureListSelections();
}

void MaterialInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    editTracker.TryCommit(liveState, [&](const MaterialInspectorState& before, const MaterialInspectorState& after) {
        if (SetMaterialInspectorCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetMaterialInspectorCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void MaterialInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void MaterialInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void MaterialInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    suppressSync = true;
    SetMaterialInspectorCommand::ApplyState(*activeCtx.target, liveState);
    suppressSync = false;
}

Utf8String* MaterialInspectorWidget::TexturePathForSlot(const MaterialTextureSlot slot) noexcept {
    switch (slot) {
        case MaterialTextureSlot::BaseColor:
            return &liveState.baseColorTexturePath;
        case MaterialTextureSlot::Normal:
            return &liveState.normalTexturePath;
        case MaterialTextureSlot::MetallicRoughness:
            return &liveState.metallicRoughnessTexturePath;
        case MaterialTextureSlot::Emissive:
            return &liveState.emissiveTexturePath;
    }
    return nullptr;
}

const Utf8String* MaterialInspectorWidget::TexturePathForSlot(const MaterialTextureSlot slot) const noexcept {
    return const_cast<MaterialInspectorWidget*>(this)->TexturePathForSlot(slot);
}

void MaterialInspectorWidget::OnTextureSlotChanged(const MaterialTextureSlot slot, const int listIndex) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target) || activeCtx.textureCatalog == nullptr) {
        return;
    }
    editTracker.BeginEdit(SetMaterialInspectorCommand::Capture(*activeCtx.target));
    Utf8String* path = TexturePathForSlot(slot);
    if (path == nullptr) {
        return;
    }
    if (listIndex <= 0) {
        *path = Utf8String{};
    } else {
        const Array<EditorTextureEntry>& entries = activeCtx.textureCatalog->GetEntries();
        const std::size_t entryIndex = static_cast<std::size_t>(listIndex - 1);
        if (entryIndex < entries.GetSize()) {
            *path = entries[entryIndex].relativePath;
        }
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MaterialInspectorWidget::OnTintChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetMaterialInspectorCommand::Capture(*activeCtx.target));
    if (axis == 0) {
        liveState.tint.x = value;
    } else if (axis == 1) {
        liveState.tint.y = value;
    } else {
        liveState.tint.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MaterialInspectorWidget::OnMetallicChanged(const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetMaterialInspectorCommand::Capture(*activeCtx.target));
    liveState.metallic = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MaterialInspectorWidget::OnRoughnessChanged(const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetMaterialInspectorCommand::Capture(*activeCtx.target));
    liveState.roughness = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MaterialInspectorWidget::OnEmissiveIntensityChanged(const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetMaterialInspectorCommand::Capture(*activeCtx.target));
    liveState.emissiveIntensity = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MaterialInspectorWidget::OnTintRChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTintChanged(0, value);
}

void MaterialInspectorWidget::OnTintGChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTintChanged(1, value);
}

void MaterialInspectorWidget::OnTintBChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTintChanged(2, value);
}

void MaterialInspectorWidget::OnMetallicSliderChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnMetallicChanged(value);
}

void MaterialInspectorWidget::OnRoughnessSliderChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnRoughnessChanged(value);
}

void MaterialInspectorWidget::OnEmissiveIntensitySliderChanged(void* const userData, const float value) {
    static_cast<MaterialInspectorWidget*>(userData)->OnEmissiveIntensityChanged(value);
}

void MaterialInspectorWidget::OnBaseColorTextureChanged(void* const userData, const int index) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTextureSlotChanged(MaterialTextureSlot::BaseColor, index);
}

void MaterialInspectorWidget::OnNormalTextureChanged(void* const userData, const int index) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTextureSlotChanged(MaterialTextureSlot::Normal, index);
}

void MaterialInspectorWidget::OnMetallicRoughnessTextureChanged(void* const userData, const int index) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTextureSlotChanged(MaterialTextureSlot::MetallicRoughness, index);
}

void MaterialInspectorWidget::OnEmissiveTextureChanged(void* const userData, const int index) {
    static_cast<MaterialInspectorWidget*>(userData)->OnTextureSlotChanged(MaterialTextureSlot::Emissive, index);
}

}  // namespace Spark::Editor
