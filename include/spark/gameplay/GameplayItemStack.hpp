#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

class GameplayItemStack final {
public:
    GameplayItemStack() noexcept = default;
    GameplayItemStack(const char* itemIdIn, const int quantityIn) noexcept
        : itemId(itemIdIn != nullptr ? itemIdIn : "item"), quantity(quantityIn > 0 ? quantityIn : 1) {}

    [[nodiscard]] const char* GetItemId() const noexcept { return itemId.CStr(); }
    [[nodiscard]] int GetQuantity() const noexcept { return quantity; }

private:
    Utf8String itemId{"item"};
    int quantity = 1;
};

}  // namespace Spark
