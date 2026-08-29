#include "spark/platform/NativeFilePicker.hpp"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <cstring>

namespace Spark {

namespace {

void ActivateAppForModalPanel() noexcept {
    if (NSApp == nil) {
        return;
    }
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

void SetPanelDirectory(NSSavePanel* panel, const char* const directoryPath) noexcept {
    if (directoryPath == nullptr || directoryPath[0] == '\0') {
        return;
    }
    NSString* path = [NSString stringWithUTF8String:directoryPath];
    if (path == nil) {
        return;
    }
    BOOL isDirectory = NO;
    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory] && isDirectory) {
        panel.directoryURL = [NSURL fileURLWithPath:path isDirectory:YES];
        return;
    }
    NSString* parent = [path stringByDeletingLastPathComponent];
    if (parent.length > 0
            && [[NSFileManager defaultManager] fileExistsAtPath:parent isDirectory:&isDirectory]
            && isDirectory) {
        panel.directoryURL = [NSURL fileURLWithPath:parent isDirectory:YES];
    }
}

void SetPanelDirectory(NSOpenPanel* panel, const char* const directoryPath) noexcept {
    if (directoryPath == nullptr || directoryPath[0] == '\0') {
        return;
    }
    NSString* path = [NSString stringWithUTF8String:directoryPath];
    if (path == nil) {
        return;
    }
    BOOL isDirectory = NO;
    if ([[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory] && isDirectory) {
        panel.directoryURL = [NSURL fileURLWithPath:path isDirectory:YES];
        return;
    }
    NSString* parent = [path stringByDeletingLastPathComponent];
    if (parent.length > 0
            && [[NSFileManager defaultManager] fileExistsAtPath:parent isDirectory:&isDirectory]
            && isDirectory) {
        panel.directoryURL = [NSURL fileURLWithPath:parent isDirectory:YES];
    }
}

void ApplySparkSceneContentTypes(NSSavePanel* panel) noexcept {
    if (@available(macOS 11.0, *)) {
        UTType* sceneType = [UTType typeWithFilenameExtension:@"sparkscene"];
        if (sceneType != nil) {
            panel.allowedContentTypes = @[ sceneType ];
        }
    } else {
        panel.allowedFileTypes = @[ @"sparkscene" ];
    }
}

void ApplySparkSceneContentTypes(NSOpenPanel* panel) noexcept {
    if (@available(macOS 11.0, *)) {
        UTType* sceneType = [UTType typeWithFilenameExtension:@"sparkscene"];
        if (sceneType != nil) {
            panel.allowedContentTypes = @[ sceneType ];
        }
    } else {
        panel.allowedFileTypes = @[ @"sparkscene" ];
    }
    panel.allowsOtherFileTypes = YES;
}

void EnsureSparkSceneExtension(Utf8String& path) noexcept {
    constexpr const char* kExtension = ".sparkscene";
    const char* const value = path.CStr();
    if (value == nullptr) {
        return;
    }
    const std::size_t length = std::strlen(value);
    const std::size_t extensionLength = std::strlen(kExtension);
    if (length >= extensionLength
            && std::strcmp(value + length - extensionLength, kExtension) == 0) {
        return;
    }
    path.AppendUtf8(kExtension);
}

bool RunOpenPanel(NSOpenPanel* panel, Utf8String& outAbsolutePath) noexcept {
    ActivateAppForModalPanel();
    if (NSWindow* keyWindow = [NSApp keyWindow]) {
        panel.level = keyWindow.level + 1;
    }
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

bool RunSavePanel(NSSavePanel* panel, Utf8String& outAbsolutePath) noexcept {
    ActivateAppForModalPanel();
    if (NSWindow* keyWindow = [NSApp keyWindow]) {
        panel.level = keyWindow.level + 1;
    }
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
    EnsureSparkSceneExtension(outAbsolutePath);
    return true;
}

}  // namespace

bool NativeFilePicker::TryPickSaveProjectFolder(Utf8String& outAbsolutePath) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = NO;
        panel.canChooseDirectories = YES;
        panel.allowsMultipleSelection = NO;
        panel.canCreateDirectories = YES;
        panel.title = @"Save Project To";
        panel.prompt = @"Save";
        return RunOpenPanel(panel, outAbsolutePath);
    }
}

bool NativeFilePicker::TryPickSaveSparkSceneFile(
        Utf8String& outAbsolutePath,
        const char* const suggestedFileName,
        const char* const defaultDirectory) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.canCreateDirectories = YES;
        ApplySparkSceneContentTypes(panel);
        panel.title = @"Save Scene";
        panel.nameFieldStringValue =
                suggestedFileName != nullptr ? [NSString stringWithUTF8String:suggestedFileName] : @"scene.sparkscene";
        SetPanelDirectory(panel, defaultDirectory);
        return RunSavePanel(panel, outAbsolutePath);
    }
}

bool NativeFilePicker::TryPickOpenSparkSceneFile(
        Utf8String& outAbsolutePath,
        const char* const defaultDirectory) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.canCreateDirectories = NO;
        ApplySparkSceneContentTypes(panel);
        panel.title = @"Open Scene";
        panel.prompt = @"Open";
        SetPanelDirectory(panel, defaultDirectory);
        return RunOpenPanel(panel, outAbsolutePath);
    }
}

bool NativeFilePicker::TryPickProjectFolder(Utf8String& outAbsolutePath) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = NO;
        panel.canChooseDirectories = YES;
        panel.allowsMultipleSelection = NO;
        panel.canCreateDirectories = YES;
        panel.title = @"Select Project Folder";
        panel.prompt = @"Open";
        return RunOpenPanel(panel, outAbsolutePath);
    }
}

bool NativeFilePicker::TryPickGltfFile(Utf8String& outAbsolutePath) noexcept {
    outAbsolutePath.Clear();
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        if (@available(macOS 11.0, *)) {
            NSMutableArray<UTType*>* types = [NSMutableArray array];
            UTType* glb = [UTType typeWithFilenameExtension:@"glb"];
            UTType* gltf = [UTType typeWithFilenameExtension:@"gltf"];
            if (glb != nil) {
                [types addObject:glb];
            }
            if (gltf != nil) {
                [types addObject:gltf];
            }
            if (types.count > 0) {
                panel.allowedContentTypes = types;
            }
        } else {
            panel.allowedFileTypes = @[ @"glb", @"gltf" ];
        }
        panel.allowsOtherFileTypes = YES;
        panel.title = @"Import glTF Model";
        panel.prompt = @"Import";
        return RunOpenPanel(panel, outAbsolutePath);
    }
}

}  // namespace Spark
