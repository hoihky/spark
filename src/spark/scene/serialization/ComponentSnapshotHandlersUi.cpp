#include "spark/scene/serialization/ComponentSnapshotHandlersUi.hpp"

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/UiWidgetSnapshot.hpp"
#include "spark/ui/core/UiTheme.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

const char* ThemePresetTag(const Ui::UiTheme& theme) noexcept {
    if (theme.controlIdleTop.x == Ui::UiTheme::ClassicMint().controlIdleTop.x &&
        theme.labelPrimary.x == Ui::UiTheme::ClassicMint().labelPrimary.x) {
        return "classic_mint";
    }
    if (theme.controlIdleTop.x == Ui::UiTheme::SceneEditorDark().controlIdleTop.x) {
        return "scene_editor_dark";
    }
    return "classic_mint";
}

Ui::UiTheme ThemeFromPresetTag(const char* tag) noexcept {
    if (tag != nullptr && std::strcmp(tag, "scene_editor_dark") == 0) {
        return Ui::UiTheme::SceneEditorDark();
    }
    return Ui::UiTheme::ClassicMint();
}

class UiCanvasSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::UiCanvas; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "ui_canvas"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const UiCanvasComponent* canvas = owner.GetComponent<UiCanvasComponent>();
        if (canvas == nullptr) {
            return false;
        }
        Utf8String payload;
        payload.AppendUtf8("v1 ");
        char header[96]{};
        std::snprintf(
                header,
                sizeof(header),
                "%d %u %u ",
                canvas->GetSortOrder(),
                canvas->IsCanvasEnabled() ? 1U : 0U,
                canvas->GetModalInputCapture() ? 1U : 0U);
        payload.AppendUtf8(header);
        ComponentSnapshotPayload::AppendQuotedString(payload, ThemePresetTag(canvas->GetTheme()));
        payload.AppendUtf8(" ");
        Utf8String widgetPayload;
        if (!Ui::UiWidgetSnapshot::TryCaptureTree(canvas->GetRoot(), widgetPayload)) {
            widgetPayload = Utf8String("none");
        }
        payload.AppendUtf8(widgetPayload);
        out.kind = Utf8String(GetKindTag());
        out.payload = MoveTemp(payload);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!ComponentSnapshotPayload::KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        const char* cursor = record.payload.CStr();
        if (std::strncmp(cursor, "v1 ", 3) != 0) {
            return false;
        }
        cursor += 3;
        int sortOrder = 0;
        unsigned enabled = 1;
        unsigned modal = 0;
        if (std::sscanf(cursor, "%d %u %u", &sortOrder, &enabled, &modal) < 3) {
            return false;
        }
        ComponentSnapshotPayload::SkipTokens(cursor, 3);
        char themeTag[64]{};
        if (!ComponentSnapshotPayload::ParseLeadingQuotedString(cursor, themeTag, sizeof(themeTag))) {
            return false;
        }

        UiCanvasComponent* canvas = owner.GetComponent<UiCanvasComponent>();
        if (canvas == nullptr) {
            canvas = owner.AddComponent<UiCanvasComponent>();
        }
        canvas->SetSortOrder(sortOrder);
        canvas->SetCanvasEnabled(enabled != 0U);
        canvas->SetModalInputCapture(modal != 0U);
        canvas->SetTheme(ThemeFromPresetTag(themeTag));

        Ui::SparkUiControlsFactory factory{};
        UniquePtr<Ui::IUiElement> root = Ui::UiWidgetSnapshot::TryRestoreTree(cursor, factory);
        if (root) {
            canvas->AdoptRoot(MoveTemp(root));
        }
        return true;
    }
};

}  // namespace

void RegisterUiSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<UiCanvasSnapshotHandler>(registry);
}

}  // namespace Spark
