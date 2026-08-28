#include "spark/platform/NativeFilePicker.hpp"

namespace Spark {

#if !defined(SPARK_PLATFORM_APPLE)

bool NativeFilePicker::TryPickGltfFile(Utf8String& /*outAbsolutePath*/) noexcept {
    return false;
}

#endif

}  // namespace Spark
