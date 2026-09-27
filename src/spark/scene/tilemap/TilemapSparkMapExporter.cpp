#include "spark/scene/tilemap/TilemapSparkMapExporter.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapDocumentSerializer.hpp"
#include "spark/scene/tilemap/TilemapFileResolve.hpp"

namespace Spark {

TilemapSparkMapExporter::Result TilemapSparkMapExporter::Save(const GameObject& owner, const char* outputPath) const noexcept {
    Options defaultOptions{};
    return Save(owner, outputPath, defaultOptions);
}

TilemapSparkMapExporter::Result TilemapSparkMapExporter::Save(
        const GameObject& owner,
        const char* outputPath,
        const Options& options) const noexcept {
    Result result{};
    if (outputPath == nullptr || outputPath[0] == '\0') {
        result.errorMessage = Utf8String("Output path is empty");
        return result;
    }

    const TilemapDocumentCapturer capturer{};
    const TilemapDocumentCapturer::Result captured = capturer.CaptureFromOwner(owner);
    if (!captured.IsSuccess()) {
        result.errorMessage = captured.GetErrorMessage();
        return result;
    }

    const TilemapEditValidator validator{};
    result.validationReport = validator.Validate(captured.GetDocument(), options.validationOptions);
    if (options.blockSaveWhenInvalid && !result.validationReport.IsClean()) {
        result.errorMessage = Utf8String("Validation failed");
        return result;
    }

    const Utf8String resolved = ResolveTilemapAssetPath(outputPath);
    const char* writePath = resolved.IsEmpty() ? outputPath : resolved.CStr();
    TilemapDocumentSerializer serializer{};
    if (!serializer.WriteToFile(captured.GetDocument(), writePath)) {
        result.errorMessage = Utf8String("Failed to write sparkmap");
        return result;
    }

    result.success = true;
    return result;
}

}  // namespace Spark
