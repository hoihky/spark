#include "spark/scene/serialization/UiWidgetSnapshot.hpp"

#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"
#include "spark/ui/spark/controls/SparkScrollPanel.hpp"

#include <cstdio>
#include <cstring>

namespace Spark::Ui {

namespace {

namespace Payload = ComponentSnapshotPayload;

void AppendCommonHeader(Utf8String& out, const char* type, const IUiElement& element) {
    out.AppendUtf8(type);
    out.AppendUtf8(" ");
    Payload::AppendQuotedString(out, element.GetId().CStr());
    char buf[32]{};
    std::snprintf(buf, sizeof(buf), " %u %u ", element.IsVisible() ? 1U : 0U, element.IsEnabled() ? 1U : 0U);
    out.AppendUtf8(buf);
}

bool ParseCommonHeader(const char*& cursor, char* typeOut, const std::size_t typeCap, char* idOut, const std::size_t idCap, bool& visible, bool& enabled) {
    if (typeCap == 0 || idCap == 0) {
        return false;
    }
    while (*cursor == ' ') {
        ++cursor;
    }
    std::size_t ti = 0;
    while (*cursor != '\0' && *cursor != ' ' && ti + 1 < typeCap) {
        typeOut[ti++] = *cursor++;
    }
    typeOut[ti] = '\0';
    unsigned vis = 1;
    unsigned en = 1;
    if (!Payload::ParseLeadingQuotedString(cursor, idOut, idCap)) {
        return false;
    }
    if (std::sscanf(cursor, "%u %u", &vis, &en) < 2) {
        return false;
    }
    visible = vis != 0U;
    enabled = en != 0U;
    Payload::SkipTokens(cursor, 2);
    return typeOut[0] != '\0';
}

void CaptureChildren(const IUiElement& element, Utf8String& out) {
    const Array<UniquePtr<IUiElement>>& children = element.GetChildren();
    char buf[24]{};
    std::snprintf(buf, sizeof(buf), "children %zu ", children.GetSize());
    out.AppendUtf8(buf);
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        Utf8String childPayload;
        if (!UiWidgetSnapshot::TryCaptureTree(children[i].Get(), childPayload)) {
            out.AppendUtf8("none ");
        } else {
            out.AppendUtf8(childPayload);
            out.AppendUtf8("; ");
        }
    }
}

bool RestoreChildren(const char*& cursor, IUiElement& parent, SparkUiControlsFactory& factory) {
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    if (std::strncmp(cursor, "children ", 9) != 0) {
        return true;
    }
    cursor += 9;
    std::size_t count = 0;
    if (std::sscanf(cursor, "%zu", &count) != 1) {
        return false;
    }
    Payload::SkipTokens(cursor, 1);
    for (std::size_t i = 0; i < count; ++i) {
        UniquePtr<IUiElement> child = UiWidgetSnapshot::TryRestoreTree(cursor, factory);
        if (!child) {
            return false;
        }
        parent.AddChild(MoveTemp(child));
        while (*cursor == ' ') {
            ++cursor;
        }
        if (*cursor == ';') {
            ++cursor;
        }
        while (*cursor == ' ') {
            ++cursor;
        }
    }
    return true;
}

void ApplyElementState(IUiElement& element, const bool visible, const bool enabled) {
    if (auto* base = dynamic_cast<UiElementBase*>(&element)) {
        base->SetVisible(visible);
        base->SetEnabled(enabled);
    }
}

}  // namespace

bool UiWidgetSnapshot::TryCaptureTree(const IUiElement* root, Utf8String& outPayload) {
    if (root == nullptr) {
        outPayload = Utf8String("none");
        return true;
    }
    Utf8String node;
    if (const auto* label = dynamic_cast<const SparkLabel*>(root)) {
        AppendCommonHeader(node, "label", *label);
        Payload::AppendQuotedString(node, label->GetText().CStr());
        char buf[16]{};
        std::snprintf(buf, sizeof(buf), " %u ", label->IsMuted() ? 1U : 0U);
        node.AppendUtf8(buf);
    } else if (const auto* button = dynamic_cast<const SparkButton*>(root)) {
        AppendCommonHeader(node, "button", *button);
        Payload::AppendQuotedString(node, button->GetLabel().CStr());
        node.AppendUtf8(" ");
    } else if (const auto* checkbox = dynamic_cast<const SparkCheckBox*>(root)) {
        AppendCommonHeader(node, "checkbox", *checkbox);
        Payload::AppendQuotedString(node, checkbox->GetLabel().CStr());
        char buf[16]{};
        std::snprintf(buf, sizeof(buf), " %u ", checkbox->GetValue() ? 1U : 0U);
        node.AppendUtf8(buf);
    } else if (const auto* slider = dynamic_cast<const SparkSlider*>(root)) {
        AppendCommonHeader(node, "slider", *slider);
        Payload::AppendQuotedString(node, slider->GetLabel().CStr());
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), " %.6f %.6f %.6f ", slider->GetValue(), slider->GetMinValue(), slider->GetMaxValue());
        node.AppendUtf8(buf);
    } else if (dynamic_cast<const SparkSeparator*>(root) != nullptr) {
        AppendCommonHeader(node, "separator", *root);
    } else if (const auto* panel = dynamic_cast<const SparkPanel*>(root)) {
        const PanelDesc desc = panel->ExportDesc();
        AppendCommonHeader(node, "panel", *panel);
        Payload::AppendQuotedString(node, desc.title.CStr());
        char buf[128]{};
        std::snprintf(
                buf,
                sizeof(buf),
                " %u %.6f %.6f %u %.6f %u ",
                desc.open ? 1U : 0U,
                desc.width,
                desc.height,
                desc.anchorRight ? 1U : 0U,
                desc.edgeMargin,
                desc.centerInParent ? 1U : 0U);
        node.AppendUtf8(buf);
    } else if (const auto* scroll = dynamic_cast<const SparkScrollPanel*>(root)) {
        const ScrollPanelDesc desc = scroll->ExportDesc();
        AppendCommonHeader(node, "scroll", *scroll);
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), "%.6f %.6f %.6f ", desc.height, desc.rowHeight, desc.verticalGap);
        node.AppendUtf8(buf);
    } else {
        return false;
    }
    CaptureChildren(*root, node);
    outPayload = MoveTemp(node);
    return true;
}

UniquePtr<IUiElement> UiWidgetSnapshot::TryRestoreTree(const char* payload, SparkUiControlsFactory& factory) {
    if (payload == nullptr) {
        return UniquePtr<IUiElement>();
    }
    const char* cursor = payload;
    while (*cursor == ' ') {
        ++cursor;
    }
    if (std::strncmp(cursor, "none", 4) == 0) {
        return UniquePtr<IUiElement>();
    }

    char type[32]{};
    char id[128]{};
    bool visible = true;
    bool enabled = true;
    if (!ParseCommonHeader(cursor, type, sizeof(type), id, sizeof(id), visible, enabled)) {
        return UniquePtr<IUiElement>();
    }

    if (std::strcmp(type, "label") == 0) {
        char text[256]{};
        unsigned muted = 0;
        if (!Payload::ParseLeadingQuotedString(cursor, text, sizeof(text))) {
            return UniquePtr<IUiElement>();
        }
        std::sscanf(cursor, "%u", &muted);
        Payload::SkipTokens(cursor, 1);
        LabelDesc desc{};
        desc.id = Utf8String(id);
        desc.text = Utf8String(text);
        desc.muted = muted != 0U;
        UniquePtr<ILabel> created = factory.CreateLabel(desc);
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "button") == 0) {
        char label[256]{};
        if (!Payload::ParseLeadingQuotedString(cursor, label, sizeof(label))) {
            return UniquePtr<IUiElement>();
        }
        ButtonDesc desc{};
        desc.id = Utf8String(id);
        desc.label = Utf8String(label);
        UniquePtr<IButton> created = factory.CreateButton(desc);
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "checkbox") == 0) {
        char label[256]{};
        unsigned value = 0;
        if (!Payload::ParseLeadingQuotedString(cursor, label, sizeof(label))) {
            return UniquePtr<IUiElement>();
        }
        std::sscanf(cursor, "%u", &value);
        Payload::SkipTokens(cursor, 1);
        CheckBoxDesc desc{};
        desc.id = Utf8String(id);
        desc.label = Utf8String(label);
        desc.value = value != 0U;
        UniquePtr<ICheckBox> created = factory.CreateCheckBox(desc);
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "slider") == 0) {
        char label[256]{};
        float value = 0.0F;
        float minValue = 0.0F;
        float maxValue = 1.0F;
        if (!Payload::ParseLeadingQuotedString(cursor, label, sizeof(label))) {
            return UniquePtr<IUiElement>();
        }
        std::sscanf(cursor, "%f %f %f", &value, &minValue, &maxValue);
        Payload::SkipTokens(cursor, 3);
        SliderDesc desc{};
        desc.id = Utf8String(id);
        desc.label = Utf8String(label);
        desc.value = value;
        desc.minValue = minValue;
        desc.maxValue = maxValue;
        UniquePtr<ISlider> created = factory.CreateSlider(desc);
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "separator") == 0) {
        SeparatorDesc desc{};
        desc.id = Utf8String(id);
        UniquePtr<ISeparator> created = factory.CreateSeparator(desc);
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "panel") == 0) {
        char title[256]{};
        unsigned open = 1;
        float width = 0.0F;
        float height = 0.0F;
        unsigned anchorRight = 0;
        float edgeMargin = 8.0F;
        unsigned center = 0;
        if (!Payload::ParseLeadingQuotedString(cursor, title, sizeof(title))) {
            return UniquePtr<IUiElement>();
        }
        std::sscanf(cursor, "%u %f %f %u %f %u", &open, &width, &height, &anchorRight, &edgeMargin, &center);
        Payload::SkipTokens(cursor, 6);
        PanelDesc desc{};
        desc.id = Utf8String(id);
        desc.title = Utf8String(title);
        desc.open = open != 0U;
        desc.width = width;
        desc.height = height;
        desc.anchorRight = anchorRight != 0U;
        desc.edgeMargin = edgeMargin;
        desc.centerInParent = center != 0U;
        UniquePtr<IPanel> created = factory.CreatePanel(desc);
        if (auto* panel = dynamic_cast<SparkPanel*>(created.Get())) {
            panel->ImportDesc(desc);
        }
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    if (std::strcmp(type, "scroll") == 0) {
        float height = 240.0F;
        float rowHeight = 0.0F;
        float vGap = 4.0F;
        std::sscanf(cursor, "%f %f %f", &height, &rowHeight, &vGap);
        Payload::SkipTokens(cursor, 3);
        ScrollPanelDesc desc{};
        desc.id = Utf8String(id);
        desc.height = height;
        desc.rowHeight = rowHeight;
        desc.verticalGap = vGap;
        UniquePtr<IScrollPanel> created = factory.CreateScrollPanel(desc);
        if (auto* scroll = dynamic_cast<SparkScrollPanel*>(created.Get())) {
            scroll->ImportDesc(desc);
        }
        ApplyElementState(*created, visible, enabled);
        RestoreChildren(cursor, *created, factory);
        return UniquePtr<IUiElement>(static_cast<IUiElement*>(created.Release()));
    }
    return UniquePtr<IUiElement>();
}

}  // namespace Spark::Ui
