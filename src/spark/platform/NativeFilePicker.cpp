#include "spark/platform/NativeFilePicker.hpp"

namespace Spark {

#if !defined(SPARK_PLATFORM_APPLE)

bool NativeFilePicker::TryPickGltfFile(Utf8String& /*outAbsolutePath*/) noexcept {
    return false;
}

bool NativeFilePicker::TryPickProjectFolder(Utf8String& /*outAbsolutePath*/) noexcept {
    return false;
}

bool NativeFilePicker::TryPickSaveProjectFolder(Utf8String& /*outAbsolutePath*/) noexcept {
    return false;
}

bool NativeFilePicker::TryPickSaveSparkSceneFile(
        Utf8String& /*outAbsolutePath*/,
        const char* /*suggestedFileName*/,
        const char* /*defaultDirectory*/) noexcept {
    return false;
}

bool NativeFilePicker::TryPickOpenSparkSceneFile(
        Utf8String& /*outAbsolutePath*/,
        const char* /*defaultDirectory*/) noexcept {
    return false;
}

#endif

}  // namespace Spark
