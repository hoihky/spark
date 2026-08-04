#pragma once

#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark::Ui {

/** Per-element persistent state keyed by <c>UiElementId</c> (ImGui-style id stack). */
class IUiStateStore {
public:
    virtual ~IUiStateStore() = default;

    virtual bool& BoolState(const UiElementId& id, const bool defaultValue) = 0;
    virtual float& FloatState(const UiElementId& id, const float defaultValue) = 0;
    virtual int& IntState(const UiElementId& id, const int defaultValue) = 0;

    virtual void ClearFrameTransient() noexcept = 0;
};

class UiStateStore final : public IUiStateStore {
public:
    bool& BoolState(const UiElementId& id, const bool defaultValue) override;
    float& FloatState(const UiElementId& id, const float defaultValue) override;
    int& IntState(const UiElementId& id, const int defaultValue) override;
    void ClearFrameTransient() noexcept override;

private:
    HashMap<Utf8String, bool, Utf8StringHash, Utf8StringEqual> boolStates{};
    HashMap<Utf8String, float, Utf8StringHash, Utf8StringEqual> floatStates{};
    HashMap<Utf8String, int, Utf8StringHash, Utf8StringEqual> intStates{};
};

}  // namespace Spark::Ui
