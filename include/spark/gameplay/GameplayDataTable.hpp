#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

/**
 * Line-oriented gameplay tuning table (no JSON dependency).
 *
 * <pre>
 * spark_gameplay_v1
 * float enemy.slime.max_hp 12
 * int gem.value 5
 * bool pickup.respawn 0
 * </pre>
 */
class GameplayDataTable final {
public:
    [[nodiscard]] bool TryLoad(const char* path) noexcept;

    [[nodiscard]] bool GetBool(const char* key, bool defaultValue) const noexcept;
    [[nodiscard]] int GetInt(const char* key, int defaultValue) const noexcept;
    [[nodiscard]] float GetFloat(const char* key, float defaultValue) const noexcept;
    [[nodiscard]] const char* GetString(const char* key, const char* defaultValue) const noexcept;

private:
    struct Entry {
        Utf8String key{};
        Utf8String value{};
    };

    Array<Entry> entries{};
};

}  // namespace Spark
