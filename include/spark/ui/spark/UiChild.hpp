#pragma once

#include "spark/core/TypeTraits.hpp"
#include "spark/ui/core/IUiElement.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Ui {

/** Adopt a factory-created control into a parent's child list. */
template<typename T>
    requires DerivedFrom<T, IUiElement>
inline void AdoptUiChild(IUiElement& parent, UniquePtr<T> child) {
    if (child) {
        parent.AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(child.Release())));
    }
}

}  // namespace Spark::Ui
