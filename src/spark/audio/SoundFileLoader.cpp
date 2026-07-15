#include "spark/audio/SoundFileLoader.hpp"

#include "spark/audio/WavDecoder.hpp"
#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstddef>

namespace Spark {

SharedPtr<SoundClip> TryLoadSoundClipFromOggFile(const char* path);
SharedPtr<SoundClip> TryLoadSoundClipFromMp3File(const char* path);

namespace {

[[nodiscard]] const char* FileExtensionDot(const char* path) noexcept {
    const char* dot = nullptr;
    for (const char* p = path; *p != '\0'; ++p) {
#ifdef _WIN32
        if (*p == '/' || *p == '\\') {
            dot = nullptr;
        }
#else
        if (*p == '/') {
            dot = nullptr;
        }
#endif
        if (*p == '.') {
            dot = p;
        }
    }
    return dot;
}

[[nodiscard]] bool ExtEqualsIgnoreCase(const char* ext, const char* ref) noexcept {
    if (ext == nullptr) {
        return false;
    }
    while (*ref != '\0') {
        char a = *ext++;
        char b = *ref++;
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return *ext == '\0';
}

[[nodiscard]] const char* BundledAssetSuffix(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return nullptr;
    }
    static const char kPrefix[] = "assets/";
    constexpr std::size_t kLen = sizeof(kPrefix) - 1U;
    for (std::size_t i = 0; i < kLen; ++i) {
        if (path[i] == '\0' || path[i] != kPrefix[i]) {
            return path;
        }
    }
    return path + kLen;
}

}  // namespace

SharedPtr<SoundClip> TryLoadSoundClipFromBundledAsset(const char* assetPathUtf8) {
    if (assetPathUtf8 == nullptr || assetPathUtf8[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    if (assetPathUtf8[0] == '/') {
        return TryLoadSoundClipFromFile(assetPathUtf8);
    }
#ifdef _WIN32
    if (assetPathUtf8[0] != '\0' && assetPathUtf8[1] == ':' &&
            (assetPathUtf8[2] == '\\' || assetPathUtf8[2] == '/')) {
        return TryLoadSoundClipFromFile(assetPathUtf8);
    }
#endif
    const char* suffix = BundledAssetSuffix(assetPathUtf8);
    if (suffix == nullptr || suffix[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    Utf8String underSource(SPARK_ASSETS_DIR);
    underSource.AppendUtf8("/");
    underSource.AppendUtf8(suffix);
    SharedPtr<SoundClip> clip = TryLoadSoundClipFromFile(underSource.CStr());
    if (clip) {
        return clip;
    }
    Utf8String underBuild(SPARK_BUILD_ASSETS_DIR);
    underBuild.AppendUtf8("/");
    underBuild.AppendUtf8(suffix);
    clip = TryLoadSoundClipFromFile(underBuild.CStr());
    if (clip) {
        return clip;
    }
    return TryLoadSoundClipFromFile(assetPathUtf8);
}

SharedPtr<SoundClip> TryLoadSoundClipFromFile(const char* utf8Path) {
    if (utf8Path == nullptr || utf8Path[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    const char* ext = FileExtensionDot(utf8Path);
    if (ext == nullptr || ext[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    if (ExtEqualsIgnoreCase(ext, ".wav") || ExtEqualsIgnoreCase(ext, ".wave")) {
        return TryLoadSoundClipFromWavFile(utf8Path);
    }
    if (ExtEqualsIgnoreCase(ext, ".ogg")) {
        return TryLoadSoundClipFromOggFile(utf8Path);
    }
    if (ExtEqualsIgnoreCase(ext, ".mp3")) {
        return TryLoadSoundClipFromMp3File(utf8Path);
    }
    return SharedPtr<SoundClip>();
}

}  // namespace Spark
