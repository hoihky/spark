#include "spark/gameplay/GameplayDataTable.hpp"

#include "spark/core/Array.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

constexpr const char* kMagic = "spark_gameplay_v1";

}  // namespace

bool GameplayDataTable::TryLoad(const char* path) noexcept {
    entries.Clear();
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    FILE* file = std::fopen(path, "r");
    if (file == nullptr) {
        return false;
    }
    char line[512]{};
    if (std::fgets(line, sizeof(line), file) == nullptr ||
        std::strncmp(line, kMagic, std::strlen(kMagic)) != 0) {
        std::fclose(file);
        return false;
    }
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        char* cursor = line;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        char type[32]{};
        char key[192]{};
        char value[192]{};
        if (std::sscanf(cursor, "%31s %191s %191s", type, key, value) != 3) {
            continue;
        }
        Entry entry{};
        entry.key = Utf8String(key);
        entry.value = Utf8String(value);
        entries.PushBack(entry);
    }
    std::fclose(file);
    return true;
}

bool GameplayDataTable::GetBool(const char* key, const bool defaultValue) const noexcept {
    if (key == nullptr) {
        return defaultValue;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].key != Utf8String(key)) {
            continue;
        }
        const char* v = entries[i].value.CStr();
        return std::strcmp(v, "1") == 0 || std::strcmp(v, "true") == 0;
    }
    return defaultValue;
}

int GameplayDataTable::GetInt(const char* key, const int defaultValue) const noexcept {
    if (key == nullptr) {
        return defaultValue;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].key == Utf8String(key)) {
            return static_cast<int>(strtol(entries[i].value.CStr(), nullptr, 10));
        }
    }
    return defaultValue;
}

float GameplayDataTable::GetFloat(const char* key, const float defaultValue) const noexcept {
    if (key == nullptr) {
        return defaultValue;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].key == Utf8String(key)) {
            return strtof(entries[i].value.CStr(), nullptr);
        }
    }
    return defaultValue;
}

const char* GameplayDataTable::GetString(const char* key, const char* defaultValue) const noexcept {
    if (key == nullptr) {
        return defaultValue;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].key == Utf8String(key)) {
            return entries[i].value.CStr();
        }
    }
    return defaultValue;
}

}  // namespace Spark
