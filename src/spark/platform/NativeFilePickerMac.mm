#include "spark/platform/NativeFilePicker.hpp"

#import <AppKit/AppKit.h>

#include <cstring>

namespace Spark {

bool NativeFilePicker::TryPickGltfFile(Utf8String& outAbsolutePath) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[ @"glb", @"gltf" ];
        panel.title = @"Import glTF Model";
        panel.prompt = @"Import";

        if ([panel runModal] != NSModalResponseOK) {
            return false;
        }
        NSURL* url = panel.URL;
        if (url == nil) {
            return false;
        }
        const char* path = url.path.UTF8String;
        if (path == nullptr || path[0] == '\0') {
            return false;
        }
        outAbsolutePath = Utf8String(path);
        return true;
    }
}

}  // namespace Spark
