#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TilemapEditValidator.hpp"

namespace Spark {

class GameObject;

/** Capture → validate → write <c>.sparkmap</c>. */
class TilemapSparkMapExporter final {
public:
    class Result {
    public:
        [[nodiscard]] bool IsSuccess() const noexcept { return success; }
        [[nodiscard]] const Utf8String& GetErrorMessage() const noexcept { return errorMessage; }
        [[nodiscard]] const TilemapEditValidationReport& GetValidationReport() const noexcept {
            return validationReport;
        }

        bool success = false;
        Utf8String errorMessage{};
        TilemapEditValidationReport validationReport{};
    };

    class Options {
    public:
        bool blockSaveWhenInvalid = false;
        TilemapEditValidator::Options validationOptions{};
    };

    [[nodiscard]] Result Save(const GameObject& owner, const char* outputPath) const noexcept;
    [[nodiscard]] Result Save(const GameObject& owner, const char* outputPath, const Options& options) const noexcept;
};

}  // namespace Spark
